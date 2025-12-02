@echo off

echo === Building Core Engine in Debug config ===
echo Configuring Core for Debug:
cmake -G Ninja -S . -B .\bin\Debug -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling Core for Debug:
cmake --build .\bin\Debug --config Debug --parallel
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Building Core Engine in Release config ===
echo Configuring Core for Release:
cmake -G Ninja -S . -B .\bin\Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling Core for Release:
cmake --build .\bin\Release --config Release --parallel
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Core Build Process Completed ===
