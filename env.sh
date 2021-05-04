#!/bin/bash

pushd edk2/
./edksetup.sh
source ./edksetup.sh
popd
pushd devenv
source ./buildenv.sh
popd
