@echo off
set OUT_DIR=..\LaserWeb4\src\lib\web-cam-cpp\
if not exist build mkdir build
if not exist %OUT_DIR% mkdir %OUT_DIR%
cd build
cmd /C em++                                                                             ^
        ../src/cam.cpp                                                                  ^
        ../src/separateTabs.cpp                                                         ^
        ../src/vCarve.cpp                                                               ^
        -I ../../boost_1_62_0                                                           ^
        -std=c++14                                                                      ^
        -fcolor-diagnostics                                                             ^
        -Wall                                                                           ^
        -Wextra                                                                         ^
        -Wno-unused-function                                                            ^
        -Wno-unused-parameter                                                           ^
        -Wno-unused-variable                                                            ^
        -Wno-logical-op-parentheses                                                     ^
        -s ALLOW_MEMORY_GROWTH=1                                                        ^
        -s ASSERTIONS=0                                                                 ^
        -s DISABLE_EXCEPTION_CATCHING=1                                                 ^
        -s EXPORT_NAME='WebCamCpp'                                                      ^
        -s EXPORTED_FUNCTIONS="['_main','_hspocket','_separateTabs','_vCarve']"         ^
        -s MODULARIZE=1                                                                 ^
        -s NO_EXIT_RUNTIME=1                                                            ^
        -s NO_FILESYSTEM=1                                                              ^
        -s RUNTIME_LOGGING=0                                                            ^
        -s SAFE_HEAP=0                                                                  ^
        -s WASM=1                                                                       ^
        -O1                                                                             ^
        -o ..\%OUT_DIR%\web-cam-cpp.js                                                  ^

cd ..
