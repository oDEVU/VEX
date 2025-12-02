@echo off
cmake -B build -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if %errorlevel% neq 0 exit /b %errorlevel%

ninja -C build
if %errorlevel% neq 0 exit /b %errorlevel%
