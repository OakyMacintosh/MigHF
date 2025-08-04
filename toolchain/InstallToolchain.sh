#!/bin/sh

# use 'export MHF_INSTALL=<newpath>' to change install path
export MHF_INSTALL=/opt/mighf

sudo mkdir -p build
gcc mighf-unknown-gnuabi-as.c -o build/mighf-unknown-gnuabi-as

sudo mkdir -p /opt/mighf
sudo cp -r ./build/* $MHF_INSTALL/
