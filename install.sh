#!/bin/bash

mkdir -p build
cd build
conan install --profile=$CONANPROFILE --build=missing ..