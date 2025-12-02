@echo off
echo  ===  STARTING FULL ENGINE BUILD ===

echo [1/3] Entering Core...
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
echo [2/3] Entering BuildTools...
pushd BuildTools
call rebuild-buildtools-windows.bat
if %errorlevel% neq 0 (
    echo Error: BuildTools build failed!
    popd
    exit /b %errorlevel%
)
popd
echo BuildTools build successful.

:: 3. Build Editor
echo.
echo [3/3] Entering Editor...
pushd Editor
call rebuild-editor-windows.bat
if %errorlevel% neq 0 (
    echo Error: Editor build failed!
    popd
    exit /b %errorlevel%
)
popd
echo Editor build successful.

echo  ===  ALL MODULES BUILT SUCCESSFULLY  ===
pause
