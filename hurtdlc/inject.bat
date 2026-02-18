@echo off
title HurtDLC Injector
color 0B

echo.
echo  ========================================
echo    HurtDLC - Minecraft Cheat v1.0
echo  ========================================
echo.

REM Очистить старые файлы из temp
set TEMP_DIR=%TEMP%\HurtDLC
if exist "%TEMP_DIR%" (
    echo [*] Cleaning old files...
    rd /s /q "%TEMP_DIR%" 2>nul
)

REM Создать папку
mkdir "%TEMP_DIR%" 2>nul

REM Копировать новые файлы
echo [*] Copying HurtDLC files...
copy /Y "hurtdlc.dll" "%TEMP_DIR%\hurtdlc.dll" >nul
if errorlevel 1 (
    echo [!] Error: hurtdlc.dll not found!
    pause
    exit /b 1
)

copy /Y "500.ttf" "%TEMP_DIR%\500.ttf" >nul 2>nul

echo [+] Files ready in: %TEMP_DIR%
echo.
echo [*] Starting injector...
echo.

REM Инжектить
inject.exe "%TEMP_DIR%\hurtdlc.dll"

echo.
echo  ========================================
pause
