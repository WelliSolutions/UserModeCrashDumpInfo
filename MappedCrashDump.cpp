#include "MappedCrashDump.h"

#include <iostream>
#include <utility>

MappedCrashDump::MappedCrashDump(HandleWrapper fileHandle, HandleWrapper mappingHandle, LPVOID baseAddress): fileHandle_(std::move(fileHandle)), mappingHandle_(std::move(mappingHandle)), baseAddress_(baseAddress)
{
}

MappedCrashDump::~MappedCrashDump()
{
	if (baseAddress_ != nullptr) {
		UnmapViewOfFile(baseAddress_);
	}
}

MappedCrashDump::MappedCrashDump(MappedCrashDump&& other) noexcept: fileHandle_(std::move(other.fileHandle_)), mappingHandle_(std::move(other.mappingHandle_)), baseAddress_(other.baseAddress_)
{
	other.baseAddress_ = nullptr;
}

MappedCrashDump& MappedCrashDump::operator=(MappedCrashDump&& other) noexcept
{
	if (this != &other) {
		if (baseAddress_ != nullptr) {
			UnmapViewOfFile(baseAddress_);
		}
		fileHandle_ = std::move(other.fileHandle_);
		mappingHandle_ = std::move(other.mappingHandle_);
		baseAddress_ = other.baseAddress_;
		other.baseAddress_ = nullptr;
	}
	return *this;
}

LPVOID MappedCrashDump::get() const
{
	return baseAddress_;
}

MappedCrashDump::operator bool() const
{
	return baseAddress_ != nullptr;
}

MappedCrashDump MappedCrashDump::MapDumpFile(const wchar_t* dumpFilePath) {
	HandleWrapper hFile(CreateFileW(dumpFilePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
	if (!hFile) {
		std::wcerr << L"Failed to open dump file: " << GetLastError() << '\n';
		return { HandleWrapper(INVALID_HANDLE_VALUE), HandleWrapper(INVALID_HANDLE_VALUE), nullptr };
	}

	HandleWrapper hMapping(CreateFileMappingW(hFile.get(), nullptr, PAGE_READONLY, 0, 0, nullptr));
	if (!hMapping) {
		std::wcerr << L"Failed to create file mapping: " << GetLastError() << '\n';
		return { HandleWrapper(INVALID_HANDLE_VALUE), HandleWrapper(INVALID_HANDLE_VALUE), nullptr };
	}

	LPVOID baseAddress = MapViewOfFile(hMapping.get(), FILE_MAP_READ, 0, 0, 0);
	if (!baseAddress) {
		std::wcerr << L"Failed to map view of file: " << GetLastError() << '\n';
		return { HandleWrapper(INVALID_HANDLE_VALUE), HandleWrapper(INVALID_HANDLE_VALUE), nullptr };
	}

	return { std::move(hFile), std::move(hMapping), baseAddress };
}