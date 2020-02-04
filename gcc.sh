docker run -it --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook gcc -S -masm=intel $1
cat test.s
