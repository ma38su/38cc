#!/bin/sh

#docker run --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook make $1
docker run --rm -v $PWD:/38cc -w /38cc compilerbook make $1
