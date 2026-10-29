cmake -B build -S .
cmake --build build -j$(nproc)
./build/blue-hub