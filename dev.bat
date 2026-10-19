@echo off
setlocal
rem =====================================================================
rem  dev.bat - build/flash entry for this repo
rem            ESP32-S3 / ESP-IDF + esp-adf + qumios-dev
rem
rem  This script only dispatches. Real work is done by tools already in
rem  the repo:
rem    build / clean : esp-idf\tools\idf.py  (build / fullclean / clean / app-flash)
rem    full flash    : esptool shipped with esp-idf, addresses are read from
rem                    apps\build\flash_args
rem
rem  The repo's own export.bat is NOT used: on this machine the installed
rem  openocd-esp32 / riscv32-esp-elf-gdb versions do not match what this
rem  IDF branch expects, so export aborts with "Activation script failed".
rem  Environment variables are set here instead.
rem
rem  NOTE: this file must stay pure ASCII. The cmd session codepage here is
rem  65001 while the system ACP is 936, so CJK text in a .bat line gets
rem  mis-decoded as stray bytes and breaks parsing. Keep messages ASCII only
rem  and avoid parentheses in output text (cmd treats them as block markers).
rem =====================================================================

rem ---------------- environment ----------------
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
if not defined IDF_TOOLS_PATH set "IDF_TOOLS_PATH=C:\esp32\esp32_combo\.espressif"
if not exist "%IDF_TOOLS_PATH%\python_env" set "IDF_TOOLS_PATH=%USERPROFILE%\.espressif"

rem ADF_PATH / IDF_PATH must use forward slashes, otherwise CMake reports
rem "Invalid character escape" while processing qumios-dev include paths
set "ADF_PATH=%ROOT:\=/%"
set "IDF_PATH=%ADF_PATH%/esp-idf"
set "APPS=%ROOT%/apps"
set "BUILD_DIR=%APPS%/build"
set "BUILD_DIR_WIN=%ROOT%\apps\build"
set "IDF_PY=%IDF_PATH%/tools/idf.py"
set "PYTHON=%IDF_TOOLS_PATH%\python_env\idf5.4_py3.14_env\Scripts\python.exe"
set "IDF_PYTHON_ENV_PATH=%IDF_TOOLS_PATH%\python_env\idf5.4_py3.14_env"

rem cross toolchain and build tools on PATH
set "TC=%IDF_TOOLS_PATH%\tools"
set "PATH=%TC%\xtensa-esp-elf\esp-14.2.0_20241119\xtensa-esp-elf\bin;%TC%\cmake\3.30.2\bin;%TC%\ninja\1.12.1;%TC%\esp32ulp-elf\2.38_20240113\esp32ulp-elf\bin;%PATH%"
set "ESP_ROM_ELF_DIR=%TC%\esp-rom-elfs\20241011"

rem tunables
set "CHIP=esp32s3"
set "BAUD=921600"
set "APP_BIN=vb701n.bin"

rem ---------------- argument parsing ----------------
set "ACTION=%~1"
if "%ACTION%"=="" set "ACTION=help"
shift
set "PORT="
set "DO_ERASE=0"
set "CLEAN_OBJ=0"

:parse
if "%~1"=="" goto :dispatch
if /i "%~1"=="-p" (
    set "PORT=%~2"
    shift
    shift
    goto :parse
)
if /i "%~1"=="erase" (
    set "DO_ERASE=1"
    shift
    goto :parse
)
if /i "%~1"=="obj" (
    set "CLEAN_OBJ=1"
    shift
    goto :parse
)
echo [dev] unknown argument: %~1
call :usage
exit /b 1

rem ---------------- dispatch ----------------
:dispatch
if not exist "%PYTHON%" goto :nopython
if not exist "%IDF_PY%" goto :noidfpy

if /i "%ACTION%"=="build"    goto :build
if /i "%ACTION%"=="rebuild"  goto :rebuild
if /i "%ACTION%"=="clean"    goto :clean
if /i "%ACTION%"=="flash"    goto :flash
if /i "%ACTION%"=="flashall" goto :flashall
if /i "%ACTION%"=="help"     goto :help

echo [dev] unknown action: %ACTION%
call :usage
exit /b 1

:help
call :usage
exit /b 0

:nopython
echo [dev] python env not found: %PYTHON%
echo [dev] check IDF_TOOLS_PATH - now: %IDF_TOOLS_PATH%
exit /b 1

:noidfpy
echo [dev] idf.py not found: %IDF_PY%
echo [dev] esp-idf submodule missing? run: git submodule update --init esp-idf
exit /b 1

rem ---------------- build: incremental ----------------
:build
call :header "BUILD - incremental - idf.py build"
call :banner
"%PYTHON%" "%IDF_PY%" -C "%APPS%" build
if errorlevel 1 goto :failbuild
echo [dev] build done. output dir: %BUILD_DIR%
exit /b 0

:failbuild
echo [dev] build failed.
exit /b 1

rem ---------------- rebuild: full ----------------
rem NOTE: do NOT use "idf.py fullclean" here: it also runs remove_managed_components,
rem which verifies then deletes apps\managed_components\78__esp-opus (committed in this
rem repo). We only want the build cache gone, so remove the directory directly.
:rebuild
call :header "REBUILD - full - remove build dir + idf.py build"
call :banner
call :rmbuild
if errorlevel 1 goto :failrmbuild
"%PYTHON%" "%IDF_PY%" -C "%APPS%" build
if errorlevel 1 goto :failbuild
echo [dev] rebuild done. compile_commands.json for clangd is regenerated too.
exit /b 0

:failrmbuild
echo [dev] failed to remove the build dir.
exit /b 1

rem ---------------- clean ----------------
:clean
call :header "CLEAN - build cache - remove build dir"
call :banner
if "%CLEAN_OBJ%"=="1" goto :clean_obj
call :rmbuild
if errorlevel 1 goto :failclean
goto :clean_done

:clean_obj
echo [dev] idf.py clean - object files only, sdkconfig kept
"%PYTHON%" "%IDF_PY%" -C "%APPS%" clean
if errorlevel 1 goto :failclean

:clean_done
echo [dev] clean done. the build dir is gone, compile_commands.json comes back with next build.
exit /b 0

:failclean
echo [dev] clean failed.
exit /b 1

rem ---------------- flash: app partition only ----------------
:flash
call :header "FLASH - app partition only"
call :banner
call :needport
if errorlevel 1 exit /b 1
if not exist "%BUILD_DIR%\%APP_BIN%" goto :noapp
echo [dev] idf.py app-flash - address taken from flash_args
"%PYTHON%" "%IDF_PY%" -C "%APPS%" -p %PORT% app-flash
if errorlevel 1 goto :failflash
echo [dev] flash done.
exit /b 0

:noapp
echo [dev] app binary not found: %BUILD_DIR%\%APP_BIN%
echo [dev] build first:  dev.bat build
exit /b 1

:failflash
echo [dev] flash failed on port %PORT%
exit /b 1

rem ---------------- flashall: base layer then app, multi-write, no merge ----------------
:flashall
call :header "FLASHALL - base layer first, then app - multi-write, no merge"
call :banner
call :needport
if errorlevel 1 exit /b 1
if not exist "%BUILD_DIR%\flash_args" goto :noflashargs

rem addresses are read from the flash_args file generated by IDF
set "ADDR_BOOT="
set "ADDR_PART="
set "ADDR_OTA="
set "ADDR_PHY="
set "ADDR_APP="
for /f "usebackq tokens=1,2" %%a in ("%BUILD_DIR%\flash_args") do (
    if /i "%%b"=="bootloader/bootloader.bin"           set "ADDR_BOOT=%%a"
    if /i "%%b"=="partition_table/partition-table.bin" set "ADDR_PART=%%a"
    if /i "%%b"=="ota_data_initial.bin"                set "ADDR_OTA=%%a"
    if /i "%%b"=="phy_init_data.bin"                   set "ADDR_PHY=%%a"
    if /i "%%b"=="%APP_BIN%"                           set "ADDR_APP=%%a"
)
if "%ADDR_BOOT%"=="" goto :badargs
if "%ADDR_PART%"=="" goto :badargs
if "%ADDR_OTA%"=="" goto :badargs
if "%ADDR_PHY%"=="" goto :badargs
if "%ADDR_APP%"=="" goto :badargs

echo [dev] port %PORT%   chip %CHIP%   baud %BAUD%
echo [dev] stage 1 base : %ADDR_BOOT% bootloader.bin  /  %ADDR_PART% partition-table.bin
echo [dev]               %ADDR_OTA% ota_data_initial.bin  /  %ADDR_PHY% phy_init_data.bin
echo [dev] stage 2 app  : %ADDR_APP% %APP_BIN%
echo.
if "%DO_ERASE%"=="1" goto :flashall_erase
goto :flashall_stage1

:flashall_erase
echo [dev] erase_flash first - this clears nvs and saved wifi config
"%PYTHON%" -m esptool --chip %CHIP% --port %PORT% --baud %BAUD% erase_flash
if errorlevel 1 goto :failerase
goto :flashall_stage1

:failerase
echo [dev] erase failed.
exit /b 1

:flashall_stage1
echo [dev] stage 1/2: writing base layer ...
"%PYTHON%" -m esptool --chip %CHIP% --port %PORT% --baud %BAUD% write_flash --flash_mode dio --flash_freq 80m --flash_size detect ^
 %ADDR_BOOT% "%BUILD_DIR%\bootloader\bootloader.bin" ^
 %ADDR_PART% "%BUILD_DIR%\partition_table\partition-table.bin" ^
 %ADDR_OTA% "%BUILD_DIR%\ota_data_initial.bin" ^
 %ADDR_PHY% "%BUILD_DIR%\phy_init_data.bin"
if errorlevel 1 goto :failbase

echo [dev] stage 2/2: writing app ...
"%PYTHON%" -m esptool --chip %CHIP% --port %PORT% --baud %BAUD% write_flash --flash_mode dio --flash_freq 80m --flash_size detect ^
 %ADDR_APP% "%BUILD_DIR%\%APP_BIN%"
if errorlevel 1 goto :failapp
echo [dev] flashall done.
exit /b 0

:noflashargs
echo [dev] flash_args not found: %BUILD_DIR%\flash_args
echo [dev] build first:  dev.bat build
exit /b 1

:badargs
echo [dev] cannot parse addresses from %BUILD_DIR%\flash_args
echo [dev] app binary is matched as %APP_BIN%; update APP_BIN if the project was renamed.
exit /b 1

:failbase
echo [dev] base layer write failed.
exit /b 1

:failapp
echo [dev] app write failed.
exit /b 1

rem ---------------- helpers ----------------
:rmbuild
if not exist "%BUILD_DIR_WIN%" exit /b 0
echo [dev] removing the build dir
rmdir /s /q "%BUILD_DIR_WIN%"
if exist "%BUILD_DIR_WIN%" goto :rmbuild_fail
exit /b 0

:rmbuild_fail
echo [dev] could not remove the build dir
exit /b 1

:needport
if not "%PORT%"=="" exit /b 0
if defined DEV_PORT set "PORT=%DEV_PORT%"
if not "%PORT%"=="" exit /b 0
echo [dev] missing serial port - use:  dev.bat flash -p COMx
echo [dev] or set env DEV_PORT. list ports in PowerShell: [System.IO.Ports.SerialPort]::GetPortNames
exit /b 1

:banner
echo [dev] project: %ROOT%
echo [dev] IDF_PATH=%IDF_PATH%
echo [dev] ADF_PATH=%ADF_PATH%
echo [dev] tools  : %IDF_TOOLS_PATH%
exit /b 0

:header
echo.
echo ======================================================================
echo  [dev] %~1
echo ======================================================================
exit /b 0

rem ---------------- usage ----------------
:usage
echo.
echo dev.bat - build/flash entry - dispatches to idf.py / esptool in this repo
echo.
echo   dev.bat build                    incremental build - idf.py build
echo   dev.bat rebuild                  full build - remove build dir then idf.py build
echo   dev.bat clean                    clear build cache - remove the whole build dir
echo   dev.bat clean obj                object files only - idf.py clean, sdkconfig kept
echo   dev.bat flash -p COMx            download - app partition only, idf.py app-flash
echo   dev.bat flashall -p COMx         full flash - base layer then app, multi-write, no merge
echo   dev.bat flashall -p COMx erase   erase whole flash before full flash
echo   dev.bat help                     show this help
echo.
echo note: flash/flashall need -p PORT or env DEV_PORT; baud is BAUD in this script.
echo       flashall uses esptool with addresses read from apps\build\flash_args.
echo.
exit /b 0
