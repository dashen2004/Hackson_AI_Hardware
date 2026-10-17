@echo off
setlocal EnableExtensions

rem Fetch this repository's submodules, then install tools for the local ESP-IDF checkout.
set "ROOT_DIR=%~dp0"
if "%ROOT_DIR:~-1%"=="\" set "ROOT_DIR=%ROOT_DIR:~0,-1%"
set "ADF_PATH=%ROOT_DIR%"
set "IDF_PATH=%ROOT_DIR%\esp-idf"
set "EXIT_CODE=0"

where.exe git.exe >nul 2>&1
if errorlevel 1 (
    echo ERROR: git.exe was not found in PATH.
    echo Install Git for Windows and reopen this terminal.
    set "EXIT_CODE=1"
    goto :failed
)

git -C "%ROOT_DIR%" rev-parse --show-toplevel >nul 2>&1
if errorlevel 1 (
    echo ERROR: This directory is not a Git working tree.
    echo Clone it with submodules, or run this script from a working tree.
    set "EXIT_CODE=1"
    goto :failed
)

if not exist "%ROOT_DIR%\.gitmodules" (
    echo ERROR: Cannot find the repository submodule manifest: "%ROOT_DIR%\.gitmodules"
    set "EXIT_CODE=1"
    goto :failed
)

python.exe --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: python.exe was not found in PATH.
    echo Install a supported Python version for ESP-IDF and reopen this terminal.
    echo Disable the Microsoft Store Python execution alias if it takes precedence.
    set "EXIT_CODE=1"
    goto :failed
)

if not exist "%IDF_PATH%\install.bat" (
    echo INFO: ESP-IDF submodule is not initialized. It will be fetched now.
)

echo.
echo [1/2] Initializing repository submodules...
echo Sources are taken from the URLs configured in "%ROOT_DIR%\.gitmodules".
git -C "%ROOT_DIR%" submodule update --init --recursive --depth 1
if errorlevel 1 (
    echo ERROR: Failed to initialize Git submodules.
    echo Check network access and the submodule URLs in .gitmodules.
    set "EXIT_CODE=1"
    goto :failed
)

if not exist "%IDF_PATH%\install.bat" (
    echo ERROR: ESP-IDF install script is missing: "%IDF_PATH%\install.bat"
    set "EXIT_CODE=1"
    goto :failed
)

python.exe "%IDF_PATH%\tools\python_version_checker.py"
if errorlevel 1 (
    echo ERROR: The installed Python version is not supported by this ESP-IDF checkout.
    set "EXIT_CODE=1"
    goto :failed
)

echo.
echo [2/2] Installing tools for the local ESP-IDF checkout...
echo ESP-IDF source: "%IDF_PATH%"
call "%IDF_PATH%\install.bat"
set "IDF_INSTALL_EXIT_CODE=%ERRORLEVEL%"
if not "%IDF_INSTALL_EXIT_CODE%"=="0" (
    echo ERROR: ESP-IDF tool installation failed.
    set "EXIT_CODE=%IDF_INSTALL_EXIT_CODE%"
    goto :failed
)

if not exist "%ROOT_DIR%\qumios-dev\components\qm_agent\include\qm_agent.h" (
    echo ERROR: qumios-dev was not initialized correctly.
    set "EXIT_CODE=1"
    goto :failed
)

echo.
echo Repository setup completed successfully.
echo Run build.bat next to configure, build, erase, flash, and monitor the app.
endlocal
exit /b 0

:failed
if "%EXIT_CODE%"=="0" set "EXIT_CODE=1"
echo.
echo Repository setup failed with exit code %EXIT_CODE%.
endlocal & exit /b %EXIT_CODE%
