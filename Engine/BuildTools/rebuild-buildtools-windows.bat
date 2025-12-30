@echo off
cmake -B build -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

ninja -C build
if %errorlevel% neq 0 exit /b %errorlevel%

echo Checking PATH...
echo %PATH% | findstr /I /C:"%BUILD_DIR%" >nul
if %errorlevel% neq 0 (
    echo Adding %BUILD_DIR% to User PATH...
    for /f "tokens=2*" %%A in ('reg query "HKCU\Environment" /v Path') do set "OLD_PATH=%%B"
    setx PATH "%BUILD_DIR%;%OLD_PATH%"
    echo [+] Path updated. Please restart your terminal for changes to take effect.
) else (
    echo [!] Build directory is already in PATH.
)
