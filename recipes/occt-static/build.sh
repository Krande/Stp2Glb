# extract tar.gz to this dir
tar --strip-components=1 -xvzf occt-${PKG_VERSION}.tar.gz

cmake -S . -B build  -G Ninja \
      -D CMAKE_FIND_ROOT_PATH="$PREFIX;$BUILD_PREFIX/$HOST/sysroot;/usr" \
      -D CMAKE_INSTALL_PREFIX:FILEPATH=$PREFIX \
      -D CMAKE_PREFIX_PATH:FILEPATH=$PREFIX \
      -D 3RDPARTY_DIR:FILEPATH=$PREFIX \
      -D BUILD_MODULE_Draw:BOOL=OFF \
      -D USE_TBB:BOOL=OFF \
      -D CMAKE_BUILD_TYPE:STRING="Release" \
      -D BUILD_RELEASE_DISABLE_EXCEPTIONS=OFF \
      -D USE_VTK:BOOL=OFF \
      -D USE_FREEIMAGE:BOOL=ON \
      -D USE_RAPIDJSON:BOOL=ON \
      -D BUILD_RELEASE_DISABLE_EXCEPTIONS:BOOL=OFF \
      -D BUILD_LIBRARY_TYPE="Static" \
      -D CMAKE_EXE_LINKER_FLAGS="-lpthread -ldl -lm" \
      -D BUILD_SHARED_LIBS:BOOL=OFF

cmake --build build -- install
