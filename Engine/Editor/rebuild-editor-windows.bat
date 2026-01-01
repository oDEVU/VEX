@echo off

echo === Building Editor ===
echo Configuring Editor for Debug:
cmake -G Ninja -S . -B .\bin -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling Editor:
cmake --build .\bin --config Debug --parallel
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Editor Build Process Completed ===

echo.
echo [1] Create 'Vex Engine' Desktop Shortcut?
echo [2] Add Vex binaries to Command Line (PATH)?
echo [3] Do both
echo [4] Do nothing
set /P c=Select option [1-4]:

if "%c%"=="1" goto :SHORTCUT
if "%c%"=="2" goto :PATH
if "%c%"=="3" goto :BOTH
goto :END

:BOTH
call :SHORTCUT_LOGIC
call :PATH_LOGIC
goto :END

:SHORTCUT
call :SHORTCUT_LOGIC
goto :END

:PATH
call :PATH_LOGIC
goto :END


:SHORTCUT_LOGIC
echo Creating Vex Engine Shortcut...
set "TARGET_EXE=%cd%\bin\VexProjectSelector.exe"
set "SHORTCUT_PATH=%USERPROFILE%\Desktop\Vex Engine.lnk"
set "ICON_PATH=%cd%\bin\icon.ico"
set "WORK_DIR=%cd%\bin"
powershell -Command "$s=(New-Object -COM WScript.Shell).CreateShortcut('%SHORTCUT_PATH%');$s.TargetPath='%TARGET_EXE%';$s.WorkingDirectory='%WORK_DIR%';$s.IconLocation='%ICON_PATH%';$s.Save()"
echo [+] Shortcut created.
exit /b 0

:PATH_LOGIC
echo Adding bin directory to User PATH...

:: Get the current directory and normalize it (remove trailing backslash)
set "ADD_PATH=%cd%\bin"
if "%ADD_PATH:~-1%"=="\" set "ADD_PATH=%ADD_PATH:~0,-1%"

:: Call PowerShell safely to append to user PATH
powershell -NoProfile -Command "$addPath='%ADD_PATH%'; $oldPath=[Environment]::GetEnvironmentVariable('Path','User'); if([string]::IsNullOrEmpty($oldPath)) { $newPath=$addPath } elseif(-not ($oldPath.Split(';') -contains $addPath)) { $newPath=$oldPath + ';' + $addPath } else { Write-Host '[-] Already in PATH.'; exit }; [Environment]::SetEnvironmentVariable('Path', $newPath,'User'); Write-Host '[+] Added to PATH safely.'; Write-Host '(Note: Restart terminal for PATH changes to take effect)'"

exit /b 0

:END
pause
