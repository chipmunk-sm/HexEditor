
@echo off
@echo.
@echo ******************************************************
@echo *** Setup environment ***
@echo ******************************************************
@echo.

if [%APPVEYOR_BUILD_FOLDER%] == [] set APPVEYOR_BUILD_FOLDER=%cd%

if [%QTDIR%] == [] set QTDIR=D:\Qt\5.10.1\msvc2015_64
if [%QTDIR%] == [] set QTDIR=D:\Qt\5.10.1\msvc2015

if [%CONFIGURATION%] == [] set CONFIGURATION=Release

rem if [%APPVEYOR_BUILD_VERSION%] == [] set APPVEYOR_BUILD_VERSION=0.0.0

if "%VCPATH14%" == "" (
    if EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
       set VCPATH14="%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC"
    ) else  (
       set VCPATH14="D:\MVS2015\VC"
    )
)

if "%VCPATH15%" == "" (
    if EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio 15.0\VC\vcvarsall.bat" (
       set VCPATH15="%ProgramFiles(x86)%\Microsoft Visual Studio 15.0\VC"
    ) else (
       set VCPATH15="D:\MVS2017\VC"
    )
)

@echo CONFIGURATION = %CONFIGURATION%
@echo APPVEYOR_BUILD_FOLDER = %APPVEYOR_BUILD_FOLDER%
@echo QTDIR = %QTDIR%
@echo VCPATH14 = %VCPATH14%
@echo VCPATH15 = %VCPATH15%

@echo.
@echo ******************************************************
@echo *** Remove temporary folders
@echo ******************************************************
@echo.

rmdir /Q /S "%APPVEYOR_BUILD_FOLDER%-build"
rmdir /Q /S "%APPVEYOR_BUILD_FOLDER%\Installer\Hexeditor"
mkdir "%APPVEYOR_BUILD_FOLDER%-build"

@echo.
@echo ******************************************************
@echo *** Setup 7z ***
@echo ******************************************************
@echo.

where 7z.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 set PATH=%PATH%;C:\Program Files\7-Zip

@echo PATH=%PATH%

where 7z.exe >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    @echo 7z.exe not found in path!
    GOTO error
)

@echo.
@echo ******************************************************
@echo *** Set Qt environment
@echo ******************************************************
@echo.

call "%QTDIR%\bin\qtenv2.bat"
if %ERRORLEVEL% NEQ 0 GOTO error

@echo.
qmake -v
if %ERRORLEVEL% NEQ 0 GOTO error

@echo.
@echo ******************************************************
@echo *** Test platform x86/x64
@echo ******************************************************
@echo.

if %QTDIR:_64=%==%QTDIR% (set ARCH=x86) else set ARCH=x64
@echo ARCH=%ARCH%

@echo.
@echo ******************************************************
@echo *** Test for VS or mingw
@echo ******************************************************
@echo.

if %QTDIR:msvc=%==%QTDIR% g++ --version
if %QTDIR:msvc=%==%QTDIR% set make=mingw32-make.exe
if %QTDIR:msvc=%==%QTDIR% %make% --version
if not %QTDIR:msvc2015=%==%QTDIR% call %VCPATH14%\vcvarsall.bat %ARCH%
if not %QTDIR:msvc2017=%==%QTDIR% call %VCPATH15%\vcvarsall.bat %ARCH%
if not %QTDIR:msvc=%==%QTDIR% set make=nmake.exe
if not %QTDIR:msvc=%==%QTDIR% %make% /? > nul
if %ERRORLEVEL% NEQ 0 GOTO error

rem ******************************************************
rem call version.ps1
rem ******************************************************

powershell.exe -noprofile -executionpolicy bypass -file "%APPVEYOR_BUILD_FOLDER%\version.ps1" -platformId %ARCH%
if %ERRORLEVEL% NEQ 0 GOTO error

set TEMP_FOLDER=%cd%
if exist "%APPVEYOR_BUILD_FOLDER%\tmpver.txt" (
	cd %APPVEYOR_BUILD_FOLDER%
	set /P APPVEYOR_BUILD_VERSION=<tmpver.txt
	cd %TEMP_FOLDER%
)

@echo. 
@echo ******************************************************
@echo *** Start build version "%APPVEYOR_BUILD_VERSION%" ***
@echo ******************************************************
@echo.

qmake -o "%APPVEYOR_BUILD_FOLDER%-build" -r -Wall -Wlogic -Wparser CONFIG+=%CONFIGURATION% %APPVEYOR_BUILD_FOLDER%
if %ERRORLEVEL% NEQ 0 GOTO error

cd "%APPVEYOR_BUILD_FOLDER%-build"

%make%
if %ERRORLEVEL% NEQ 0 GOTO error

@echo. 
@echo ******************************************************
@echo *** Collect files to "%APPVEYOR_BUILD_FOLDER%\Installer\Hexeditor\" 
@echo ******************************************************
@echo.

cd "%APPVEYOR_BUILD_FOLDER%"

windeployqt "%APPVEYOR_BUILD_FOLDER%-build\%CONFIGURATION%\hexeditor.exe" --dir Installer\Hexeditor
if %ERRORLEVEL% NEQ 0 GOTO error

copy "%APPVEYOR_BUILD_FOLDER%-build\%CONFIGURATION%\hexeditor.exe" "%APPVEYOR_BUILD_FOLDER%\Installer\Hexeditor\"
if %ERRORLEVEL% NEQ 0 GOTO error

copy "%APPVEYOR_BUILD_FOLDER%\data\*.*" "%APPVEYOR_BUILD_FOLDER%\Installer\Hexeditor\"
if %ERRORLEVEL% NEQ 0 GOTO error

powershell.exe -noprofile -executionpolicy bypass -file "%APPVEYOR_BUILD_FOLDER%\installer.ps1" -platformId %ARCH% -cultureId "en-us"
if %ERRORLEVEL% NEQ 0 GOTO error

cd "%APPVEYOR_BUILD_FOLDER%\Installer"

@echo. 
@echo ******************************************************
@echo *** Create archive ***
@echo ******************************************************
@echo.

7z.exe a "%APPVEYOR_BUILD_FOLDER%\hexeditor.%ARCH%.%APPVEYOR_BUILD_VERSION%.zip" "Hexeditor\*"
if %ERRORLEVEL% NEQ 0 GOTO error

7z.exe a "%APPVEYOR_BUILD_FOLDER%\hexeditor.%ARCH%.%APPVEYOR_BUILD_VERSION%.Installer.zip" hexeditor.msi
if %ERRORLEVEL% NEQ 0 GOTO error
  
goto exit

:error
    @echo Failed!.
	rem cd "%APPVEYOR_BUILD_FOLDER%"
	rem exit 1
:exit  
cd "%APPVEYOR_BUILD_FOLDER%"
rem pause
