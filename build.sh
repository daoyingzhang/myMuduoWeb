#!/bin/bash

set -e

SOURCE_DIR=`pwd`
# SRC_BASE=${SOURCE_DIR}/src/base
# SRC_NET=${SOURCE_DIR}/src/net

sudo rm -fr ${SOURCE_DIR}/build/*
sudo rm -fr ${SOURCE_DIR}/include/*

# 如果没有 build 目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi


cd  ${SOURCE_DIR}/build &&
    cmake .. &&
    sudo make install


# 使操作生效
# sudo ldconfig