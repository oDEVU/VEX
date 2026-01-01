@echo off
setlocal enabledelayedexpansion
goto :main

:has_cmd
where %1 >nul 2>&1
exit /b %errorlevel%

:main

echo ============================================
echo   VEX Engine - Windows Dependency Checker
echo ============================================
echo.

set FAIL=0

echo --- Core CLI Tools ---

call :has_cmd git
if errorlevel 1 (echo [ERROR] git not found & set FAIL=1) else echo [OK] git

call :has_cmd cmake
if errorlevel 1 (echo [ERROR] cmake not found & set FAIL=1) else echo [OK] cmake

call :has_cmd ninja
if errorlevel 1 (
    echo [ERROR] ninja not found in PATH
    echo         Install via: winget install Ninja-build.Ninja
    set FAIL=1
) else echo [OK] ninja

call :has_cmd clang
if errorlevel 1 (
    echo [ERROR] clang not found in PATH
    echo         Ensure LLVM bin directory is on PATH
    set FAIL=1
) else echo [OK] clang

call :has_cmd magick
if errorlevel 1 (
    echo [WARN] ImageMagick missing
) else echo [OK] ImageMagick

echo.
echo --- Vulkan SDK ---

call :has_cmd vulkaninfo
if errorlevel 1 call :has_cmd slangc

if errorlevel 1 (
    echo [ERROR] Vulkan SDK not detected in PATH
    set FAIL=1
) else echo [OK] Vulkan SDK

echo.
echo --- Windows SDK ---

set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
set "SDK_FOUND=0"

if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requiresAny Microsoft.VisualStudio.Component.Windows10SDK,Microsoft.VisualStudio.Component.Windows11SDK -property installationPath`) do (
        set "SDK_FOUND=1"
    )

    if !SDK_FOUND! == 1 (
        echo [OK] Windows SDK found
    ) else (
        echo [ERROR] Windows SDK not detected
        echo         Open Visual Studio Installer and ensure one of these is installed:
        echo         - Windows 10 SDK
        echo         - Windows 11 SDK
        set FAIL=1
    )
) else (
    echo [ERROR] Visual Studio Installer not found
    set FAIL=1
)

if !FAIL! == 0 (
    echo All dependencies detected.
) else (
    echo Dependency check failed.
)

exit /b !FAIL!
