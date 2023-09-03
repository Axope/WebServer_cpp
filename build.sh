#!/bin/bash

PWD=$(pwd)
echo pwd=$PWD

build_dir="$PWD/build/"
echo build_dir=$build_dir

if [ ! -d "$build_dir" ]; then
    mkdir "$build_dir"
    echo "Folder created: $build_dir"
else
    echo "Folder already exists: $build_dir"
fi

cd $build_dir
cmake ..
make -j4
chmod 777 server
cp server ..