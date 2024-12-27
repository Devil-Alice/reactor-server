#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <dirent.h>

int addr_len = sizeof(struct sockaddr_in);

typedef struct dir_item_info
{
    char *name;
    char *type;
    int size;
} dir_item_info;

char *content_type_map[][2] =
    {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"xml", "application/xml"},
        {"txt", "text/plain"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"ico", "image/x-icon"},
        {"svg", "image/svg+xml"},
        {"pdf", "application/pdf"},
        {"zip", "application/zip"},
        {"gz", "application/gzip"},
        {"mp3", "audio/mpeg"},
        {"mp4", "video/mp4"},
        {"wav", "audio/wav"},
        {"webm", "video/webm"},
        {"ogg", "audio/ogg"},
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"ttf", "font/ttf"},
        {"otf", "font/otf"}};

int init_listen_fd(unsigned short port)
{
    int ret = 0;

    // 创建socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket failed");
        return -1;
    }

    // 设置端口复用，此处的意义是在服务器重启时，可以立刻重新绑定到原来的地址端口，而不必等到资源释放
    int optval = 1;
    ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret == -1)
    {
        perror("setsockopt failed");
        return -1;
    }

    // 绑定端口
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    ret = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {
        perror("bind failed");
        return -1;
    }

    // 监听
    ret = listen(listen_fd, 128);
    if (ret == -1)
    {
        perror("listen failed");
        return -1;
    }

    return listen_fd;
}

int run_epoll(int listen_fd)
{
    // 创建一个epoll实例，本质就是一颗红黑树的根节点
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        perror("epoll_create failed");
        return -1;
    }

    // 将server_fd添加到epoll树上
    struct epoll_event epevent_listen;
    epevent_listen.events = EPOLLIN;
    epevent_listen.data.fd = listen_fd;
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &epevent_listen);
    if (ret == -1)
    {
        perror("epoll_ctl add listen fd failed");
        return -1;
    }

    // 使用epoll检测事件
    int epevents_capacity = 1024;
    struct epoll_event epevents[epevents_capacity];
    while (1)
    {
        // epoll_wait返回当前检测到的数量，并将触发的epoll_event放入epevents数组里
        int event_num = epoll_wait(epoll_fd, epevents, epevents_capacity, -1);
        // printf("epoll detected\n");
        // 遍历这个事件数组
        for (int i = 0; i < event_num; i++)
        {
            int fd = epevents[i].data.fd;

            // 构造一个fd_info对象，为线程函数使用
            fd_info_t *fd_info = (fd_info_t *)malloc(sizeof(fd_info_t));
            fd_info->listen_fd = listen_fd;
            fd_info->epoll_fd = epoll_fd;

            // 获取事件的data的fd，判断是否是监听的fd
            if (fd == listen_fd)
            {
                // accept_client(epoll_fd, listen_fd);
                pthread_create(&fd_info->tid, NULL, thread_accept_client, fd_info);
            }
            else
            {
                // 如果不是监听的fd，说明是其他的通信fd，在此作处理
                // recv_request(epoll_fd, fd);
                fd_info->client_fd = fd;
                pthread_create(&fd_info->tid, NULL, thread_recv_request, fd_info);
            }
        }
    }

    return 0;
}

int accept_client(int epoll_fd, int listen_fd)
{
    // 接收客户端连接
    struct sockaddr_in client_addr;
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd == -1)
    {
        perror("accept failed");
        return -1;
    }

    // epoll效率最高的方式的 非阻塞边沿触发模式，步骤：1.设置fd为非阻塞；2.设置epoll_event时，将events追加一个EPOLLET（edge trigger）

    // 设置fd为非阻塞
    int flag = fcntl(client_fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, flag);

    // 通过监听fd获得的通信fd，需要将其放入epoll树中继续监听
    struct epoll_event epevent_ =
        {
            .data.fd = client_fd,
            .events = EPOLLIN | EPOLLET};
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &epevent_);

    return client_fd;
}

char *content_type_get(const char *file_name)
{
    // 通过filename获取文件后缀
    char *dot = strrchr(file_name, '.');
    char *ext = (dot + 1);
    int map_size = sizeof(content_type_map) / sizeof(content_type_map[0]);

    for (int i = 0; i < map_size; i++)
        if (strcasecmp(ext, content_type_map[i][0]) == 0)
            return content_type_map[i][1];

    return "text/plain";
}

int handle_request(int client_fd, linked_list_t *list)
{

    // 请求行示例：

    // GET /?username=alice&password=admin HTTP/1.1
    // Host: 106.15.42.223:55555
    // User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.1.1 Safari/605.1.15
    // Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
    // Accept-Encoding: gzip, deflate
    // Accept-Language: zh-CN,zh-Hans;q=0.9
    // Priority: u=0, i
    // Upgrade-Insecure-Requests: 1

    linked_list_node_t *n = list->head;
    // 此处的list中包含了请求的所有字符，如果遇到超长字符串，需要遍历每一个元素，使用动态分配将他们拼接为一个完整的字符串
    // 拼接的原因：下面需要对请求进行解析，如果不拼接，很有可能造成字符串截断导致解析错误
    // 对拼接好的字符串使用strtok以\r\n截取子字符串

    char *requeset = NULL;
    char type[16] = {0}, path[256] = {0};

    // 遍历获取字符串，并拼接（暂未实现）
    while (n != NULL)
    {
        requeset = (char *)(n->data);
        // 输出log
        printf("request:\r\n%s\r\n", requeset);
        n = n->next;
    }

    // 对字符串进行检查
    if (requeset == NULL)
        return -1;

    // 获取请求的第一行，例如：// GET /?username=alice&password=admin HTTP/1.1
    char request_line[256] = {0};
    // 通过strstr查找子串的位置，此处也就是\r的指针位置
    char *pos = strstr(requeset, "\r\n");
    // 用查找到的指针位置减去初始位置，就是这一段字符串的长度
    int sub_str_len = pos - requeset;
    strncpy(request_line, requeset, sub_str_len);

    // 使用sscanf格式化字符串并输出到变量中
    sscanf(request_line, "%[^ ]  %[^ ] ", type, path);
    // printf(">>>>>>>>>>type: %s\n>>>>>>>>>>path: %s\r\n", type, path);

    // 检查是否是get请求，这里的对比函数不区分大小写
    if (strcasecmp(type, "get") != 0)
        return -1;

    // 将请求的资源路径前加上一个.表示当前程序的工作目录下的资源
    char res_path[257] = {0};
    res_path[0] = '.';

    // 处理默认的请求 /
    if (strcmp(path, "/") == 0)
        strcpy(path, "/index.html");

    strcat(res_path, path);
    printf("\tclient fd %d requeste path: %s\r\n\r\n", client_fd, res_path);

    // 获取文件属性
    struct stat file_info;
    int ret = stat(res_path, &file_info);
    if (ret == -1)
    {
        // 文件不存在，返回404页面
        struct stat not_found_page;
        int ret = stat("./404.html", &not_found_page);
        send_file(client_fd, "./404.html");
        perror("stat failed");
        return -1;
    }

    // 判断路径是文件还是目录,通过宏S_ISDIR判断，传入stat对象的st_mode属性，返回值为1表示为目录
    if (S_ISDIR(file_info.st_mode) == 1)
    {
        // 将目录中的文件发送给客户端
        send_dir(client_fd, res_path);
        printf("request resource is a directory\r\n");
    }
    else
    {
        // 不是目录，将文件本身发送给客户端
        send_file(client_fd, res_path);
        printf("request resource is a file\r\n");
    }
    return 0;
}

int send_file(int client_fd, char *file_name)
{
    int fd = open(file_name, O_RDONLY);
    // 这里采用断言来判断fd是否合法
    // 断言：如果传入的条件满足，不做任何事情；如果不满足，则程序会输出错误信息，然后调用abort()终止程序
    assert(fd > 0);

#if 0

    // 成功获取文件fd后，不断的读取文件内容，发送给客户端
    int len = 0;
    char buf[1024] = {0};
    while (1)
    {
        len = read(fd, buf, (sizeof buf) - 1);
        if (len <= 0)
            break;
        send(client_fd, buf, len, 0);

        // 让线程稍微休眠一会，避免客户端处理不过来
        // 这很重要！
        usleep(10);
    }

#else
    // 还有一种更简便的方法，sendfile：零拷贝的高性能api
    // 使用系统自带的函数sendifle即可完成文件的发送
    //  lseek函数本来是将文件的指针从offset移动到whence，并将该操作移动的字节数返回
    //  此处可以借用返回的特性求该文件的大小
    //  当然也可以使用之前是stat求得
    int file_size = lseek(fd, 0, SEEK_END);

    // 发送响应头
    send_response_header(client_fd, 200, "OK", content_type_get(file_name), file_size);

    // 由于上一行代码将文件指针移动到了末尾，所以要重新移到开头，才能保证接下来的文件发送可以发送完整
    lseek(fd, 0, SEEK_SET);
    off_t offset = 0;
    while (offset < file_size)
    {
        int ret = sendfile(client_fd, fd, &offset, file_size);
        if (ret == -1)
        {
            perror("sendfile failed");
            close(fd);
            return -1;
        }
    }

#endif

    close(fd);
    return 0;
}

int send_dir(int client_fd, char *dir_name)
{
    // 这里指向的是struct dirnet*类型的数组，所以这里定义二级指针
    struct dirent **name_list;
    // 使用scandir获取nameList，该文件会为传入的nameList分配空间，所以在使用完毕后，需要对nameList释放
    // 第三个参数为filter，这里不需要；第四个参数是排序函数，使用库自带的alphasort
    // 该函数返回目录下获取的name数量
    int name_cnt = scandir(dir_name, &name_list, NULL, alphasort);

    char path[1024] = {0};
    char send_buf[4096] = {0};
    // 构建json
    sprintf(send_buf, "{\"%s\":[", "name_list");
    for (int i = 0; i < name_cnt; i++)
    {
        // 取出名字
        char *name = name_list[i]->d_name;
        // 通过stat判断是否是目录
        struct stat name_info;
        // 拼接dirName+name。
        // 原因：此处获取的是dirName目录下的nameList，是基于dirName的名字
        // 所以对于当前的工作目录来说，应该是dirName+name
        sprintf(path, "%s/%s", dir_name, name);
        stat(path, &name_info);
        // 这里定义info是为了防止别的地方用到这个信息
        dir_item_info info;
        info.name = name;

        if (S_ISDIR(name_info.st_mode) == 1)
        {
            // 是目录
            info.type = "dir";
            info.size = -1;
        }
        else
        {
            // 是文件
            info.type = "file";
            info.size = name_info.st_size;
        }

        if (i == 0)
            sprintf(send_buf + strlen(send_buf), "{\"name\": \"%s\", \"type\": \"%s\", \"size\": %d}", info.name, info.type, info.size);
        else
            sprintf(send_buf + strlen(send_buf), ",{\"name\": \"%s\", \"type\": \"%s\", \"size\": %d}", info.name, info.type, info.size);
    }
    sprintf(send_buf + strlen(send_buf), "]}");

    printf("name list json: \r\n%s\r\n", send_buf);

    send_response_header(client_fd, 200, "OK", "application/json", strlen(send_buf));
    send(client_fd, send_buf, strlen(send_buf), 0);

    //  使用完毕释放
    for (int i = 0; i < name_cnt; i++)
        free(name_list[i]);

    free(name_list);

    return 0;
}

void *thread_accept_client(void *arg_fd_info)
{
    fd_info_t *fd_info = (fd_info_t *)arg_fd_info;
    accept_client(fd_info->epoll_fd, fd_info->listen_fd);
    free(arg_fd_info);
    return NULL;
}

int recv_request(int epoll_fd, int client_fd)
{
    printf("\tclient fd %d request\n", client_fd);

    linked_list_t llist = linked_list_create();

    char buf[1024] = {0};
    int len;

    while ((len = recv(client_fd, buf, (sizeof buf) - 1, 0)) > 0)
    {
        // 将每一段字符串都放入链表中
        // todo: 优化存储，这里是存的buf的地址，后续可能会被覆盖
        linked_list_push_tail(&llist, buf);
    }

    // 在非阻塞模式下读取，如果没有数据可以读（也就是读取完毕），则会返回EAGAIN错误码
    if (len == -1 && errno == EAGAIN)
    {
        // 解析请求内容
        handle_request(client_fd, &llist);
    }
    else if (len == 0)
    {
        // 客户端断开连接，需要删除对客户端fd的检测，并关闭fd
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
        printf("\tclient fd %d closed\n", client_fd);
    }

    return 0;
}

void *thread_recv_request(void *arg_fd_info)
{
    fd_info_t *fd_info = (fd_info_t *)arg_fd_info;
    recv_request(fd_info->epoll_fd, fd_info->client_fd);
    free(arg_fd_info);
    return NULL;
}

int send_response_header(int client_fd, int status, const char *descripetion, const char *type, int len)
{
    char buf[4096] = {0};
    sprintf(buf, "HTTP/1.1 %d %s\r\n", status, descripetion);
    sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);
    // sprintf(buf + strlen(buf), "Content-Type: %s\r\n", "plain/text");
    if (len != -1)
        sprintf(buf + strlen(buf), "Content-Length: %d\r\n", len);
    sprintf(buf + strlen(buf), "\r\n"); // 空行，一定要有！

    // 发送
    send(client_fd, buf, strlen(buf), 0);

    return 0;
}
