#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "HandleWrapper.h"

class MappedCrashDump {
public:
    MappedCrashDump(HandleWrapper fileHandle, HandleWrapper mappingHandle, LPVOID baseAddress);
    ~MappedCrashDump();
    MappedCrashDump(const MappedCrashDump& other) = delete;
    MappedCrashDump& operator=(const MappedCrashDump& other) = delete;
    MappedCrashDump(MappedCrashDump&& other) noexcept;
    MappedCrashDump& operator=(MappedCrashDump&& other) noexcept;

    LPVOID get() const;
    operator bool() const;

    static MappedCrashDump MapDumpFile(const wchar_t* dumpFilePath);

private:
    HandleWrapper fileHandle_;
    HandleWrapper mappingHandle_;
    LPVOID baseAddress_;
};


