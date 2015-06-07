#!/usr/bin/env bash

build_autotools() {
  autoreconf -i
  ./configure --prefix=$PWD/_install
  make install
}

build_cmake() {
  mkdir $PWD/_build
  cd $PWD/_build
  cmake .. -DCMAKE_INSTALL_PREFIX=$PWD/../_install
  cmake --build . --target install
  ctest -V .
}

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 {autotools|cmake}"
  exit 1
fi

set -e  # exit on error.
set -x  # echo commands.

eval "build_$1"
