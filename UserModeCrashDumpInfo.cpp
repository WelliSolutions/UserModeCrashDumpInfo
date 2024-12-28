#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <string>
#include <sstream>
#include <dbgeng.h>
#include <atlbase.h>
#include <iomanip>
#include <filesystem>

#include <locale>

#include "StringHelpers.h"
#include "MappedCrashDump.h"

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "dbgeng.lib")

ULONG32 ExceptionCode(const MappedCrashDump& crash_dump) {
	if (!crash_dump) {
		return -1;
	}

	PMINIDUMP_DIRECTORY exceptionStreamDirectory = nullptr;
	MINIDUMP_EXCEPTION_STREAM* exceptionStream = nullptr;
	ULONG streamSize = 0;
	if (MiniDumpReadDumpStream(crash_dump.get(), ExceptionStream, &exceptionStreamDirectory, reinterpret_cast<PVOID*>(&exceptionStream), &streamSize)) {
		return exceptionStream->ExceptionRecord.ExceptionCode;
	}
	else {
		std::wcerr << L"Failed to read exception stream: " << GetLastError() << '\n';
	}

	return -1;
}

std::wstring ExecutableName(const MappedCrashDump& crash_dump) {
	if (!crash_dump) {
		return L"";
	}

	PMINIDUMP_DIRECTORY moduleListStreamDirectory = nullptr;
	MINIDUMP_MODULE_LIST* moduleList = nullptr;
	ULONG streamSize = 0;
	if (MiniDumpReadDumpStream(crash_dump.get(), ModuleListStream, &moduleListStreamDirectory, reinterpret_cast<PVOID*>(&moduleList), &streamSize)) {
		if (moduleList->NumberOfModules > 0) {
			MINIDUMP_MODULE& module = moduleList->Modules[0];
			if (module.ModuleNameRva != 0) {
				auto pModuleNameRva = static_cast<char*>(crash_dump.get()) + module.ModuleNameRva;
				uint32_t nameLength = *reinterpret_cast<uint32_t*>(pModuleNameRva) / 2;
				const wchar_t* pModuleName = reinterpret_cast<const wchar_t*>(pModuleNameRva + sizeof(nameLength));
				return std::wstring(pModuleName, nameLength);
			}
		}
	}
	else {
		std::wcerr << L"Failed to read module list stream: " << GetLastError() << '\n';
	}

	return L"";
}

std::wstring ExecutableNameOnly(const MappedCrashDump& crash_dump) {
	std::wstring fullPath = ExecutableName(crash_dump);
	size_t pos = fullPath.find_last_of(L"\\/");
	if (pos != std::wstring::npos) {
		return fullPath.substr(pos + 1);
	}
	return fullPath;
}

struct VersionInfo {
	uint16_t major{};
	uint16_t minor{};
	uint16_t patch{};
	uint16_t build{};

	std::wstring to_string() const {
		return std::to_wstring(major) + L"." + std::to_wstring(minor) + L"." + std::to_wstring(patch) + L"." + std::to_wstring(build);
	}
};

VersionInfo FileVersion(const MappedCrashDump& crash_dump) {
	if (!crash_dump) {
		return {};
	}

	PMINIDUMP_DIRECTORY moduleListStreamDirectory = nullptr;
	MINIDUMP_MODULE_LIST* moduleList = nullptr;
	ULONG streamSize = 0;
	if (MiniDumpReadDumpStream(crash_dump.get(), ModuleListStream, &moduleListStreamDirectory, reinterpret_cast<PVOID*>(&moduleList), &streamSize)) {
		if (moduleList->NumberOfModules > 0) {
			MINIDUMP_MODULE& module = moduleList->Modules[0];
			VersionInfo version;
			version.major = module.VersionInfo.dwFileVersionMS >> 16;
			version.minor = module.VersionInfo.dwFileVersionMS & 0xFFFF;
			version.patch = module.VersionInfo.dwFileVersionLS >> 16;
			version.build = module.VersionInfo.dwFileVersionLS & 0xFFFF;
			return version;
		}
	}
	else {
		std::wcerr << L"Failed to read module list stream: " << GetLastError() << '\n';
	}

	return {};
}

VersionInfo ProductVersion(const MappedCrashDump& crash_dump) {
	if (!crash_dump) {
		return {};
	}

	PMINIDUMP_DIRECTORY moduleListStreamDirectory = nullptr;
	MINIDUMP_MODULE_LIST* moduleList = nullptr;
	ULONG streamSize = 0;
	if (MiniDumpReadDumpStream(crash_dump.get(), ModuleListStream, &moduleListStreamDirectory, reinterpret_cast<PVOID*>(&moduleList), &streamSize)) {
		if (moduleList->NumberOfModules > 0) {
			MINIDUMP_MODULE& module = moduleList->Modules[0];
			VersionInfo version;
			version.major = module.VersionInfo.dwProductVersionMS >> 16;
			version.minor = module.VersionInfo.dwProductVersionMS & 0xFFFF;
			version.patch = module.VersionInfo.dwProductVersionLS >> 16;
			version.build = module.VersionInfo.dwProductVersionLS & 0xFFFF;
			return version;
		}
	}
	else {
		std::wcerr << L"Failed to read module list stream: " << GetLastError() << '\n';
	}

	return {};
}

void UnixTimeToFileTime(time_t t, LPFILETIME pft) {
	LONGLONG result = t * 10000000i64 + 116444736000000000i64;
	pft->dwLowDateTime = static_cast<DWORD>(result);
	pft->dwHighDateTime = result >> 32;
}

std::wstring CrashTime(const MappedCrashDump& crash_dump, wchar_t* dumpfilename) {
	if (!crash_dump) {
		return L"";
	}

	CComPtr<IDebugClient5> debugClient;
	CComPtr<IDebugControl2> debugControl;

	if (DebugCreate(__uuidof(IDebugClient5), reinterpret_cast<void**>(&debugClient)) != S_OK) {
		std::wcerr << L"Failed to create debug client\n";
		return L"";
	}

	if (debugClient->QueryInterface(__uuidof(IDebugControl2), reinterpret_cast<void**>(&debugControl)) != S_OK) {
		std::wcerr << L"Failed to query IDebugControl2 interface\n";
		return L"";
	}

	if (debugClient->OpenDumpFileWide(dumpfilename, 0) != S_OK) {
		std::wcerr << L"Failed to open dump file\n";
		return L"";
	}

	if (debugControl->WaitForEvent(0, INFINITE) != S_OK) {
		std::wcerr << L"Failed to wait for event\n";
		return L"";
	}

	ULONG timeDateStamp = 0;
	if (debugControl->GetCurrentTimeDate(&timeDateStamp) != S_OK) {
		std::wcerr << L"Failed to get current time date\n";
		return L"";
	}

	FILETIME fileTime;
	SYSTEMTIME systemTime;
	UnixTimeToFileTime(timeDateStamp, &fileTime);
	FileTimeToSystemTime(&fileTime, &systemTime);

	std::wstringstream wss;
	wss << systemTime.wYear << L"-"
		<< std::setw(2) << std::setfill(L'0') << systemTime.wMonth << L"-"
		<< std::setw(2) << std::setfill(L'0') << systemTime.wDay << L" "
		<< std::setw(2) << std::setfill(L'0') << systemTime.wHour << L"-"
		<< std::setw(2) << std::setfill(L'0') << systemTime.wMinute << L"-"
		<< std::setw(2) << std::setfill(L'0') << systemTime.wSecond;

	return wss.str();
}

int wmain(int argc, wchar_t* argv[]) {
	if (argc < 2) {
		std::wcerr << L"Usage: " << argv[0] << L" <path_to_dmp_file> [options]" << '\n';
		std::wcerr << L"Options:\n";
		std::wcerr << L"       /exception        Prints exception code\n";
		std::wcerr << L"       /exe              Prints executable name only\n";
		std::wcerr << L"       /exe_full         Prints full executable path\n";
		std::wcerr << L"       /fileversion      Prints file version\n";
		std::wcerr << L"       /productversion   Prints product version\n";
		std::wcerr << L"       /time             Prints crash dump time\n";
		std::wcerr << L"       /space            Prints space between arguments instead of newlines\n";
		std::wcerr << L"                         This is useful if you want to generate a file name\n";
		std::wcerr << L"       /rename           Renames the input file according to the output of this program\n";
		std::wcerr << L"                         This implies /space\n";
		std::wcerr << L"\n";
		std::wcerr << L"Example: " << argv[0] << L" dump.dmp /exe /exception /fileversion /time /rename\n";
		std::wcerr << L"Bulk processing:\n";
		std::wcerr << L"for %f in (*.dmp) do \"" << argv[0] << L"\" \"%f\" /exe /exception /fileversion /time /rename\n";
		return 1;
	}

	bool rename_file = false;
	std::wstringstream rename_stream;

	{
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (FAILED(hr)) {
			std::wcerr << L"Failed to initialize COM library\n";
			return 1;
		}

		auto crash_dump = MappedCrashDump::MapDumpFile(argv[1]);
		if (!crash_dump) {
			CoUninitialize();
			return 1;
		}

		bool use_space = false;

		for (int i = 2; i < argc; ++i) {
			if (wcscmp(argv[i], L"/space") == 0) {
				use_space = true;
			}
			else if (wcscmp(argv[i], L"/rename") == 0) {
				rename_file = true;
				use_space = true;
			}
		}

		for (int i = 2; i < argc; ++i) {
			if (wcscmp(argv[i], L"/exception") == 0) {
				std::wcout << std::hex << L"0x" << ExceptionCode(crash_dump);
				if (rename_file) rename_stream << std::hex << L"0x" << ExceptionCode(crash_dump);
			}
			else if (wcscmp(argv[i], L"/exe") == 0) {
				std::wcout << ExecutableNameOnly(crash_dump);
				if (rename_file) rename_stream << ExecutableNameOnly(crash_dump);
			}
			else if (wcscmp(argv[i], L"/exe_full") == 0) {
				std::wcout << ExecutableName(crash_dump);
				if (rename_file) rename_stream << ExecutableNameOnly(crash_dump);
			}
			else if (wcscmp(argv[i], L"/fileversion") == 0) {
				std::wcout << FileVersion(crash_dump).to_string();
				if (rename_file) rename_stream << FileVersion(crash_dump).to_string();
			}
			else if (wcscmp(argv[i], L"/productversion") == 0) {
				std::wcout << ProductVersion(crash_dump).to_string();
				if (rename_file) rename_stream << ProductVersion(crash_dump).to_string();
			}
			else if (wcscmp(argv[i], L"/time") == 0) {
				std::wcout << CrashTime(crash_dump, argv[1]);
				if (rename_file) rename_stream << CrashTime(crash_dump, argv[1]);
			}
			else if (wcscmp(argv[i], L"/space") == 0 || wcscmp(argv[i], L"/rename") == 0) {
				continue; // already handled
			}
			else {
				std::wcerr << L"Unknown argument: " << argv[i] << '\n';
			}

			if (use_space) {
				std::wcout << L' ';
				if (rename_file) rename_stream << L' ';
			}
			else {
				std::wcout << L'\n';
				if (rename_file) rename_stream << L'_';
			}
		}

		CoUninitialize();
	}

	if (rename_file) {
		std::filesystem::path old_path(argv[1]);
		auto filename = trim(rename_stream.str()) + L".dmp";
		std::filesystem::path new_path = old_path.parent_path() / filename;
		if (old_path == new_path) {
			std::wcout << L"New file name is the same as the old file name. Skipping rename.\n";
			return 0;
		}

		int counter = 1;
		while (exists(new_path)) {
			new_path = old_path.parent_path() / (trim(rename_stream.str()) + L" (" + std::to_wstring(counter) + L").dmp");
			counter++;
		}

		std::error_code ec;
		std::filesystem::rename(old_path, new_path, ec);
		if (ec) {
			std::wcerr << L"Failed to rename file: " << ec.message().c_str() << '\n';
			return 1;
		}
	}

	return 0;
}
