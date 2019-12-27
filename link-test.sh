#!/bin/bash

gcc -S test/foo.c
./mcc "foo();" > call_foo.s
gcc -o tmp foo.s call_foo.s
./tmp

gcc -S test/bar.c
./mcc "bar(3, 4);" > call_bar.s
gcc -o tmp bar.s call_bar.s
./tmp

rm *.s
