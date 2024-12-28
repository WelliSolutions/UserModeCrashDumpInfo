# User Mode Crash Dump Info

This is a tool for roughly analyzing Windows user mode crash dump files (.DMP) and getting basic info like:

* Exception code
* Executable name
* File version (of the executable)
* Product version (of the executable)
* Time (of the crash)

It can rename the crash dump, so that you can pick an interesting one easily, based on above criteria.

## Command line options

```none
Usage: UserModeCrashDumpInfo.exe <path_to_dmp_file> [options]
Options:
       /exception        Prints exception code
       /exe              Prints executable name only
       /exe_full         Prints full executable path
       /fileversion      Prints file version
       /productversion   Prints product version
       /time             Prints crash dump time
       /space            Prints space between arguments instead of newlines
                         This is useful if you want to generate a file name
       /rename           Renames the input file according to the output of this program
                         This implies /space

Example: UserModeCrashDumpInfo.exe dump.dmp /exe /exception /fileversion /time /rename
Bulk processing:
for %f in (*.dmp) do "UserModeCrashDumpInfo.exe" "%f" /exe /exception /fileversion /time /rename
```

The exception will be printed as an 8 digit hex number with leading 0x (e.g. `0x00000000`).

The time will be printed as an ISO like yyyy-MM-dd HH-mm-ss (not using colons, since those are not valid in file names).

## Example

### Printing basic information about the crash

```none
C:\...>UserModeCrashDumpInfo.exe 757120f3-b864-43f1-a4b0-959500f9e3d9.dmp /exception /exe /fileversion /productversion /time
0xc0000005
soffice.bin
7.0.5.2
7.0.5.2
2022-03-24 18-55-09
```

So this was an Access Violation in Libre Office 7.0.5.2 from 2022-03-24.

### Renaming the file

```none
C:\...>UserModeCrashDumpInfo.exe 757120f3-b864-43f1-a4b0-959500f9e3d9.dmp /exe /exception /time /rename
soffice.bin 0xc0000005 2022-03-24 18-55-09

C:\...>dir /b *.dmp
soffice.bin 0xc0000005 2022-03-24 18-55-09.dmp
```