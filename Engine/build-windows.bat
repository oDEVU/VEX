@echo off
echo  ===  STARTING FULL ENGINE BUILD ===

echo [1/2] Entering Core...
pushd Core
call rebuild-core-windows.bat
if %errorlevel% neq 0 (
    echo Error: Core build failed!
    popd
    exit /b %errorlevel%
)
popd
echo Core build successful.

:: 2. Build BuildTools
echo.
echo [2/2] Entering BuildTools...
pushd BuildTools
call rebuild-buildtools-windows.bat
if %errorlevel% neq 0 (
    echo Error: BuildTools build failed!
    popd
    exit /b %errorlevel%
)
popd
echo BuildTools build successful.

echo  ===  ALL MODULES BUILT SUCCESSFULLY  ===
pause
