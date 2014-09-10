
set PATH=%PATH%;C:\Program Files (x86)\CMake 2.8\bin

set CMAKE_BUILDDIR=build\vc
set STARTDIR=%CD%

@if NOT "%VS100COMNTOOLS%"=="" (call "%VS100COMNTOOLS%\vsvars32.bat" & goto VS_END)
@if NOT "%VS90COMNTOOLS%"=="" (call "%VS90COMNTOOLS%\vsvars32.bat" & goto VS_END)
@if NOT "%VS80COMNTOOLS%"=="" (call "%VS80COMNTOOLS%\vsvars32.bat" & goto VS_END)
@if NOT "%VS71COMNTOOLS%"=="" (call "%VS71COMNTOOLS%\vsvars32.bat" & goto VS_END)

:VS_END

cd %STARTDIR%

set CMAKE_BUILDDIR=build\vc
mkdir %CMAKE_BUILDDIR%

cd %CMAKE_BUILDDIR%

del CMakeCache.txt

cmake -D STMD_MSVC_MT:BOOL=OFF  %STARTDIR%
rem -D stmd_DOCS:bool=ON -D stmd_WITH_TESTS:bool=ON -DCXXTEST_INCLUDE_DIR=%STARTDIR%\3rdparty\cxxtest -DCXXTEST_PYTHON_TESTGEN_EXECUTABLE=%STARTDIR%\3rdparty\cxxtest\bin\cxxtestgen 
cpack --config CPackSourceConfig.cmake
cd %STARTDIR%
cmake --build %CMAKE_BUILDDIR% --target all --target test --target package 
rem --target package_source


pause
