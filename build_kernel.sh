#!/bin/bash

pushd kernel

if [ $1 = "clean" ]; then
  make clean
elif [ $1 = "f" ]; then
  make clean
  make
else
  make
fi

popd
