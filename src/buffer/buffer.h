#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>

class Buffer
{
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    const char *Peek() const;
    void EnsureWriteable(size_t len);
    void HasWritten(size_t len);

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PadBytes() const;

    void MoveReadPtr(size_t len);
    void MoveReadPtrToPos(const char *end);

    void ResetPtr();
    std::string GetReadableStrAndClear();

    const char *BeginWriteConst() const;
    char *BeginWrite();

    void Append(const Buffer &buff);
    void Append(const std::string &str);
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);

    ssize_t ReadFd(int fd, int *Errno);
    ssize_t WriteFd(int fd, int *Errno);

private:
    char *BeginPtr_();
    const char *BeginPtr_() const;
    // 扩容
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;
    std::atomic<std::size_t> writePos_;
};

#endif // BUFFER_H