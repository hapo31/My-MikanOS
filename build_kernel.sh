#!/bin/bash

pushd kernel

if [ $1 = "clean" ]; then
  make clean
fi
make

popd
