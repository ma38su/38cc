echo $1
docker run -it --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook gcc -S -masm=intel $1 -o $1.s
cat $1.s
