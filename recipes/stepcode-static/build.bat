cmake -S . -B build  -G Ninja ^
    -D CMAKE_BUILD_TYPE="Release" ^
    -D CMAKE_PREFIX_PATH:FILEPATH="%LIBRARY_PREFIX%" ^
    -D CMAKE_LIBRARY_PATH:FILEPATH="%LIBRARY_PREFIX%/lib" ^
    -D CMAKE_INSTALL_PREFIX:FILEPATH="%LIBRARY_PREFIX%" ^
    -D BUILD_SHARED_LIBS=OFF ^
    -D BUILD_STATIC_LIBS=ON ^
    -D CMAKE_C_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG" ^
    -D CMAKE_CXX_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG"

if errorlevel 1 exit 1

cmake --build build -- -v install

if errorlevel 1 exit 1