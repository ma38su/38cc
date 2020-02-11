echo $1
docker run -it --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook gcc -S -masm=intel $1.c -o $1.s
cat $1.s
docker run -it --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook gcc $1.s -o $1
docker run -it --rm -v $HOME/Documents/mos/cc:/38cc -w /38cc compilerbook ./$1
