@echo OFF

echo "Building OpenCASCADE Technology"
set FNAME=occt-%PKG_VERSION%.tar.gz

REM Extract the .tar.gz file to the current directory
7z x %FNAME% -aoa -o.

REM Extract the .tar file to the current directory
7z x occt-%PKG_VERSION%.tar -aoa -o. occt*

REM Copy the contents from occt*/ to the current directory
for /D %%d in (occt*) do xcopy "%%d\*" . /E /H /K /Y /Q

cmake -S . -B build  -G Ninja ^
      -D CMAKE_PREFIX_PATH:FILEPATH="%LIBRARY_PREFIX%" ^
      -D CMAKE_LIBRARY_PATH:FILEPATH="%LIBRARY_PREFIX%/lib" ^
      -D CMAKE_INSTALL_PREFIX:FILEPATH="%LIBRARY_PREFIX%" ^
      -D INSTALL_DIR_LAYOUT="Unix" ^
      -D BUILD_MODULE_Draw:BOOL=OFF ^
      -D 3RDPARTY_DIR:FILEPATH="%LIBRARY_PREFIX%" ^
      -D CMAKE_BUILD_TYPE="Release" ^
      -D USE_TBB:BOOL=OFF ^
      -D USE_VTK:BOOL=OFF ^
      -D USE_FREEIMAGE:BOOL=OFF ^
      -D USE_RAPIDJSON:BOOL=ON ^
      -D BUILD_LIBRARY_TYPE="Static" ^
      -D BUILD_RELEASE_DISABLE_EXCEPTIONS:BOOL=OFF ^
      -D CMAKE_C_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG" ^
      -D CMAKE_CXX_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG"

if errorlevel 1 exit 1

cmake --build build -- -v install

if errorlevel 1 exit 1
