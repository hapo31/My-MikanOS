#!/bin/bash

git submodule update --init
pushd edk2
git checkout refs/tags/edk2-stable202102
popd
ln -s ./MikanLoaderPkg ./edk2/
