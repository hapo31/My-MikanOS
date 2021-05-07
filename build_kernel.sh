#!/bin/bash

pushd kernel

if [ $1 = "clean" ]; then
  make clean
else
  make
fi

popd
