#!/bin/sh

HOST_SYSTEM=`uname -s`
HOST_ARCH=`uname -m`
STARTDIR=`pwd`

CMAKE_BUILDDIR=build/$HOST_ARCH-$HOST_SYSTEM

mkdir -p $CMAKE_BUILDDIR

cd $CMAKE_BUILDDIR

if test $HOST_SYSTEM = "Darwin" ; then
	cmake -DChronolog_WITH_DOCS:bool=ON \
          -DChronolog_WITH_TESTS:bool=ON \
          -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk \
          $STARTDIR;
    cd $STARTDIR

    mkdir -p build/Xcode
    cd build/Xcode
	cmake -DChronolog_WITH_DOCS:bool=ON \
          -DChronolog_WITH_TESTS:bool=ON \
          -DCXXTEST_INCLUDE_DIR=3rdparty/cxxtest \
          -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.8.sdk \
          -G Xcode $STARTDIR;
   cd $STARTDIR

elif test $HOST_SYSTEM = "Linux" ; then
	cmake -DChronolog_WITH_DOCS:bool=ON \
          -DChronolog_WITH_TESTS:bool=ON \
          $STARTDIR;
else
    echo "not supported";
fi

cd $CMAKE_BUILDDIR
cpack --config CPackSourceConfig.cmake
cmake --build . --target all --target test --target package 
cmake --build . --target package_source
cd $STARTDIR

