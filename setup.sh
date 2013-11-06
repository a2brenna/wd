#!/bin/bash

#Install pre-requisite packages on a debian based system

apt-get install python python-daemon python-psutil protobuf-compiler python-protobuf

git submodule init
git submodule update

make
