./compile.sh;
./dicc test.c;
echo -e '\n\n\n';
gcc test.s -o test;
./test;
echo 'ret value:' $?;
rm ./test ./test.s;
