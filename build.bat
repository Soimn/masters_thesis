@echo off

setlocal

cd %~dp0

if not exist build mkdir build
cd build

if "%Platform%" neq "x64" (
	echo ERROR: Platform is not "x64" - please run this from the MSVC x64 native tools command prompt.
	goto end
)

set "common_compile_options= /nologo /W3"
set "common_link_options= /incremental:no /opt:ref /subsystem:windows user32.lib ole32.lib mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib mfsensorgroup.lib"

if "%1"=="debug" (
  set "compile_options=%common_compile_options% /Od /Zo /Z7 /RTC1 /MTd /DHOLO_DEBUG#1"
  set "link_options=%common_link_options% libucrtd.lib libvcruntimed.lib"
) else if "%1"=="release" (
  set "compile_options=%common_compile_options% /O2"
  set "link_options=%common_link_options% libvcruntime.lib"
) else (
  goto invalid_arguments
)

if "%2" neq "" goto invalid_arguments

cl %compile_options% ..\src\example.c /link %link_options% /pdb:holo_example.pdb /out:holo_example.exe

cl /LD /nologo /W3 ..\src\holo_cam\dllmain.c /Od /Zo /Z7 /RTC1 /MTd /DHOLO_DEBUG#1 /link /DEF:..\src\holo_cam\holo_cam.def /incremental:no /opt:ref libucrtd.lib libvcruntimed.lib user32.lib ole32.lib mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib mfsensorgroup.lib advapi32.lib /pdb:holo_cam.pdb /out:holo_cam.dll

goto end

:invalid_arguments
echo Invalid arguments^. Usage: build ^[debug^|release^]
goto end

:end
endlocal
