cmake -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --config Release --parallel $(nproc --all)
