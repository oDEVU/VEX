@echo off
cmake -B build -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

ninja -C build
if %errorlevel% neq 0 exit /b %errorlevel%

echo Checking PATH...
powershell -NoProfile -Command ^
    "$targetPath = '%BUILD_DIR%'; " ^
    "$currentPath = [Environment]::GetEnvironmentVariable('Path', 'User'); " ^
    "if (-not ($currentPath.Split(';') -contains $targetPath)) { " ^
    "    Write-Host 'Adding BuildTools to User PATH...'; " ^
    "    $newPath = $currentPath + ';' + $targetPath; " ^
    "    [Environment]::SetEnvironmentVariable('Path', $newPath, 'User'); " ^
    "    Write-Host '[+] Path updated safely.'; " ^
    "} else { " ^
    "    Write-Host '[!] Build directory is already in PATH.'; " ^
    "}"

echo === BuildTools Build Complete ===
endlocal
