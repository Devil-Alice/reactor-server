#pragma once
#include <thread>
#include <mutex>

class Dynamic_buffer
{
private:
    char *data;
    int capacity;
    int read_pos;
    int write_pos;
    std::mutex mutex;

public:
    Dynamic_buffer(int capasicy);
    ~Dynamic_buffer();

    int get_read_pos() { return read_pos; }
    void set_read_pos(int new_pos) { read_pos = new_pos; }
    int get_write_pos() { return write_pos; }
    void set_write_pos(int new_pos) { write_pos = new_pos; }
    std::mutex *get_mutex() { return &mutex; }

    /**
     * @brief 计算可用的写空间
     * @note 此处可用的写空间，指的是包括释放后的已读空间
     */
    int available_write_size() { return capacity - write_pos; }
    int available_read_size() { return write_pos - read_pos; }
    /// @brief 获取可用的读空间，也就是返回当前读位置的指针
    char *availabel_read_data() { return data + read_pos; }
    /// @brief 获取可用的写空间，也就是返回当前写位置的指针
    char *availabel_write_data() { return data + write_pos; }
    /**
     * @brief 写入字符串，并根据当前的dynamic_buffer的使用情况，决定是否扩容
     * @note
     * 总是write_pos >= read_pos，因为无法读取还未写入的数据。
     * 如果已经读取了一部分内容，那么则可以将读取过的内容作为新空间使用。
     * 例如：| 已读 | 已写 | 空闲 | -> | 已写 | 已读(释放) + 空闲 |
     *
     * 所以，有以下几种情况：
     * 1. 剩余空间够用，不整理空间，也不扩容
     * 2. 剩余空间不够，整理之后，再扩容
     */
    int append_string(const char *str);
    /// @brief 追加数据到dynamic_buffer中，需要指定数据长度
    /// @param dynamic_buffer 目的地
    /// @param buf 写入的数据
    /// @param buf_size 数据长度
    /// @return
    int append_string(const char *str, int str_len);
    /// @param size 新的字符串的大小
    int expand(int str_len);
    /// @brief 查找字符串str在dynamic_buffer.data中第一次出现的位置
    /// @return 返回该位置的地址
    /// @note 该函数会在最后更新read_pos为 read_pos+查找到的字符串位置+条件字符串的长度
    /// @note 这么做是因为该函数返回一个可读的字符串，视为读操作，读过的字符串自然要从缓冲区中取出，方便后续的读操作
    char *find_pos(std::string str);
};
