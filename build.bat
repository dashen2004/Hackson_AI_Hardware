@echo off
setlocal EnableExtensions

rem Build the application in apps from any working directory.
set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
set "ADF_PATH=%ROOT_DIR%"
set "IDF_PATH=%ROOT_DIR%\esp-idf"
set "APP_DIR=%ROOT_DIR%\apps"
set "BUILD_EXIT_CODE="
set "ERASE_EXIT_CODE="
set "FLASH_MONITOR_EXIT_CODE="
set "EXIT_CODE="

if not exist "%ADF_PATH%\export.bat" (
    echo ERROR: Cannot find the repository export script: "%ADF_PATH%\export.bat"
    exit /b 1
)
if not exist "%APP_DIR%\CMakeLists.txt" (
    echo ERROR: Cannot find the apps project at "%APP_DIR%"
    exit /b 1
)

echo.
echo [1/4] Exporting ESP-IDF and applying ADF patches...
call "%ADF_PATH%\export.bat"
if errorlevel 1 goto :failed

echo.
echo [2/4] Building the apps project...
pushd "%APP_DIR%"
if errorlevel 1 goto :failed
idf.py build
set "BUILD_EXIT_CODE=%ERRORLEVEL%"
if not "%BUILD_EXIT_CODE%"=="0" (
    popd
    goto :failed
)
popd

echo.
echo [3/4] Erasing the chip...
pushd "%APP_DIR%"
if errorlevel 1 goto :failed
idf.py erase-flash
set "ERASE_EXIT_CODE=%ERRORLEVEL%"
popd
if not "%ERASE_EXIT_CODE%"=="0" (
    goto :failed
)

echo.
echo [4/4] Flashing the application and starting the serial monitor...
pushd "%APP_DIR%"
if errorlevel 1 goto :failed
idf.py flash monitor
set "FLASH_MONITOR_EXIT_CODE=%ERRORLEVEL%"
popd
if not "%FLASH_MONITOR_EXIT_CODE%"=="0" (
    goto :failed
)

echo.
echo Build, flash, and monitor completed successfully.
echo Output: "%APP_DIR%\build"
endlocal
exit /b 0

:failed
if defined FLASH_MONITOR_EXIT_CODE (
    set "EXIT_CODE=%FLASH_MONITOR_EXIT_CODE%"
) else if defined ERASE_EXIT_CODE (
    set "EXIT_CODE=%ERASE_EXIT_CODE%"
) else if defined BUILD_EXIT_CODE (
    set "EXIT_CODE=%BUILD_EXIT_CODE%"
) else (
    set "EXIT_CODE=%ERRORLEVEL%"
)
if "%EXIT_CODE%"=="0" set "EXIT_CODE=1"
echo.
echo Build failed with exit code %EXIT_CODE%.
endlocal & exit /b %EXIT_CODE%
