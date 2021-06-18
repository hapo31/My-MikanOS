#!/bin/bash

for MK in $(ls apps/*/Makefile)
do
  APP_DIR=$(dirname $MK)
  APP=$(basename $APP_DIR)
  make ${MAKE_OPTS:-} -C $APP_DIR $APP
done

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
