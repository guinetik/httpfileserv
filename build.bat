@echo off
REM Updated build script for platform-specific files

echo Creating build directories...
if not exist bin mkdir bin
if not exist obj mkdir obj
if not exist obj\platform mkdir obj\platform
if not exist obj\platform\windows mkdir obj\platform\windows
if not exist obj\platform\unix mkdir obj\platform\unix

echo Setting up Visual Studio environment...
call "D:\tools\BuildTools\Common7\Tools\VsDevCmd.bat" -no_logo

echo Compiling platform-specific files...
echo - Windows implementation
cl /nologo /W3 /O2 /I"include" /D_CRT_SECURE_NO_WARNINGS /c /Foobj\platform\windows\platform_windows.obj src\platform\windows\platform_windows.c
if %ERRORLEVEL% NEQ 0 goto build_error

echo Compiling main source files...
echo - httpfileserv.c
cl /nologo /W3 /O2 /I"include" /D_CRT_SECURE_NO_WARNINGS /c /Foobj\httpfileserv.obj src\httpfileserv.c
if %ERRORLEVEL% NEQ 0 goto build_error

echo - utils.c
cl /nologo /W3 /O2 /I"include" /D_CRT_SECURE_NO_WARNINGS /c /Foobj\utils.obj src\utils.c
if %ERRORLEVEL% NEQ 0 goto build_error

echo - httpfileserv_lib.c
cl /nologo /W3 /O2 /I"include" /D_CRT_SECURE_NO_WARNINGS /c /Foobj\httpfileserv_lib.obj src\httpfileserv_lib.c
if %ERRORLEVEL% NEQ 0 goto build_error

echo - http_response.c
cl /nologo /W3 /O2 /I"include" /D_CRT_SECURE_NO_WARNINGS /c /Foobj\http_response.obj src\http_response.c
if %ERRORLEVEL% NEQ 0 goto build_error

echo Linking...
link /NOLOGO /OUT:bin\httpfileserv.exe obj\httpfileserv.obj obj\utils.obj obj\httpfileserv_lib.obj obj\http_response.obj obj\platform\windows\platform_windows.obj ws2_32.lib
if %ERRORLEVEL% NEQ 0 goto build_error

echo Build SUCCESSFUL!
echo Binary created at bin\httpfileserv.exe
echo.
echo Run with: run_server.bat [directory_to_serve]
goto end

:build_error
echo Build FAILED!
exit /b 1

:end