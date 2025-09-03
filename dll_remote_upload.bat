@echo off
setlocal enabledelayedexpansion

:: --- Configuration ---
set SERVER_IP=192.168.0.180
set SERVER_PORT=22
set USERNAME=ireoplus
set LOCAL_SEARCH_DIR=%~dp0out
set REMOTE_TARGET_DIR=/home/%USERNAME%/rpvmds/mod
set SFTP_COMMAND_FILE=sftp_commands.txt
:: ----------------------

echo [INFO] Starting DLL/SO Upload Process.
echo [INFO] TARGET SFTP SERVER INFO: %USERNAME%@%SERVER_IP%/%REMOTE_TARGET_DIR%

:: 1. Check if psftp.exe is installed, if not, try to install it via winget
where psftp.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo [INFO] psftp.exe not found.
    
    :: Check if winget is available
    where winget >nul 2>nul
    if %errorlevel% neq 0 (
        echo [ERROR] winget command not found. PuTTY cannot be installed automatically.
        echo [GUIDE] Please install PuTTY manually from: https://www.putty.org/
        goto :end
    )
    
    echo [INFO] Attempting to install PuTTY using winget. This may require administrator privileges.
    winget install --id PuTTY.PuTTY --silent --accept-source-agreements --accept-package-agreements
    
    if %errorlevel% neq 0 (
        echo [ERROR] PuTTY installation failed. Please try installing it manually.
        goto :end
    )
    
    echo [INFO] PuTTY has been installed successfully.
    echo [GUIDE] Please RE-RUN this script in a NEW command prompt window for the changes to take effect.
    goto :end
)

echo.
echo [INFO] Searching .dll, .so Files and Preparing to Transfer...

if exist %SFTP_COMMAND_FILE% del %SFTP_COMMAND_FILE%
echo cd "%REMOTE_TARGET_DIR%" >> %SFTP_COMMAND_FILE%
set found_files=0
for /r "%LOCAL_SEARCH_DIR%" %%F in (*.dll, *.so) do (
    echo put "%%F" "%%~nxF" >> %SFTP_COMMAND_FILE%
    echo [INFO] File found: "%%F"
    set /a found_files+=1
)
if %found_files% equ 0 (
    echo [INFO] No .dll, .so Files in Local Directory.
    goto :cleanup
)
echo quit >> %SFTP_COMMAND_FILE%

echo.
echo [INFO] Starting File Transfer.

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Invalid Password or Connection Failed.
    goto :cleanup
)

:success
echo [INFO] File Transfer Completed.
:cleanup
if exist %SFTP_COMMAND_FILE% del %SFTP_COMMAND_FILE%

echo [INFO] Shutdown DLL/SO Upload Process.
:end
endlocal
pause