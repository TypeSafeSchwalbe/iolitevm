
clear

mkdir out > /dev/null 2>&1

gcc -Wextra src/*.c -o out/iolitevm -lm -O3 &&
out/iolitevm -mods test.iob -slibs testdlib/out/testdlib.so -start main