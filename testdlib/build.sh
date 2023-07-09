
mkdir out > /dev/null 2>&1
gcc -c -Wall -Werror -fpic src/*.c -o out/testdlib.o && gcc -shared out/testdlib.o -o out/testdlib.so