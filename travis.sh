#!/usr/bin/env bash

build_autotools() {
  autoreconf -i
  ./configure --prefix=$PWD/_install
  make install
}

build_cmake() {
  CFLAGS="-fsanitize=undefined"
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/../_install
  cmake --build . --target install
  ctest -V .
}

build_asan() {
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/../_install \
    -DCMAKE_C_FLAGS="-fsanitize=address,undefined -O1"
  cmake --build . --target install
  ctest -V .
}

build_tsan() {
  mkdir $PWD/_build && cd $PWD/_build
  cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/../_install \
    -DCMAKE_C_FLAGS="-fsanitize=thread,undefined -O1"
  cmake --build . --target install
  ctest -V .
}

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 {autotools|cmake|asan|tsan}"
  exit 1
fi

set -e  # exit on error.
set -x  # echo commands.

eval "build_$1"
