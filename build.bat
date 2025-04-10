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

if "%1"=="debug" (
  set "compile_options=%common_compile_options% /Od /Zo /Z7 /RTC1 /MTd /DHOLO_DEBUG#1"
) else if "%1"=="release" (
  set "compile_options=%common_compile_options% /O2"
) else (
  goto invalid_arguments
)

if "%2" neq "" goto invalid_arguments

set "holo_example_dependencies=user32.lib ole32.lib mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib mfsensorgroup.lib Ws2_32.lib"
cl %compile_options% ..\src\example.c /link /incremental:no /opt:ref /pdb:holo_example.pdb /out:holo_example.exe %holo_example_dependencies%

set "holo_cam_dependencies=libucrtd.lib libvcruntimed.lib user32.lib ole32.lib mf.lib mfplat.lib mfuuid.lib mfreadwrite.lib mfsensorgroup.lib advapi32.lib d2d1.lib Ws2_32.lib"
cl /LD /nologo /W3 ..\src\holo_cam\dllmain.c /Od /Zo /Z7 /RTC1 /MTd /DHOLO_DEBUG#1 /link /DEF:..\src\holo_cam\holo_cam.def /incremental:no /opt:ref  /pdb:holo_cam.pdb /out:holo_cam.dll %holo_cam_dependencies%

copy holo_cam.dll ..\src\holo_cam_python\holocam\.
copy ..\src\holo_cam.h ..\src\holo_cam_python\holocam\.

goto end

:invalid_arguments
echo Invalid arguments^. Usage: build ^[debug^|release^]
goto end

:end
endlocal
