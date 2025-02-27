cmake -S . -B build  -G Ninja \
    -D SC_BUILD_TYPE="Release" \
    -D CMAKE_BUILD_TYPE="Release" \
    -D CMAKE_PREFIX_PATH:FILEPATH="${PREFIX}" \
    -D CMAKE_LIBRARY_PATH:FILEPATH="${PREFIX}/lib" \
    -D CMAKE_INSTALL_PREFIX:FILEPATH="${PREFIX}" \
    -D BUILD_SHARED_LIBS=OFF \
    -D ENABLE_ALL=ON \
    -D BUILD_STATIC_LIBS=ON \
    -D SC_CPP_GENERATOR:BOOL=ON \
    -D SC_PYTHON_GENERATOR:BOOL=ON

cmake --build build -- -v install
