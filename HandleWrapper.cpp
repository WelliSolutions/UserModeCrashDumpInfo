#include "HandleWrapper.h"

HandleWrapper::HandleWrapper(HANDLE handle): handle_(handle)
{}

HandleWrapper::~HandleWrapper()
{
	if (handle_ != INVALID_HANDLE_VALUE) {
		CloseHandle(handle_);
	}
}

HandleWrapper::HandleWrapper(HandleWrapper&& other) noexcept: handle_(other.handle_)
{
	other.handle_ = INVALID_HANDLE_VALUE;
}

HandleWrapper& HandleWrapper::operator=(HandleWrapper&& other) noexcept
{
	if (this != &other) {
		if (handle_ != INVALID_HANDLE_VALUE) {
			CloseHandle(handle_);
		}
		handle_ = other.handle_;
		other.handle_ = INVALID_HANDLE_VALUE;
	}
	return *this;
}

HANDLE HandleWrapper::get() const
{
	return handle_;
}

HandleWrapper::operator bool() const
{
	return handle_ != INVALID_HANDLE_VALUE;
}
