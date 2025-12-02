@echo off

echo === Building Editor ===
echo Configuring Editor for Debug:
cmake -G Ninja -S . -B .\bin -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling Editor:
cmake --build .\bin --config Debug --parallel
if %errorlevel% neq 0 exit /b %errorlevel%

echo === Editor Build Process Completed ===
