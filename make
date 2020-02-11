#!/bin/sh

docker run --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook make $1
