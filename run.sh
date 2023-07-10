
clear

mkdir out > /dev/null 2>&1
gcc -Wextra src/*.c -o out/iolitevm -lm && out/iolitevm
