#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class HandleWrapper {
public:
    explicit HandleWrapper(HANDLE handle);
    ~HandleWrapper();
    HandleWrapper(const HandleWrapper& other) = delete;
    HandleWrapper& operator=(const HandleWrapper& other) = delete;
    HandleWrapper(HandleWrapper&& other) noexcept;
    HandleWrapper& operator=(HandleWrapper&& other) noexcept;

    HANDLE get() const;
    operator bool() const;

private:
    HANDLE handle_;
};

