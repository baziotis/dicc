# So that you can compile in the same place as the executable
# make -C ./src
# make clean -C ./src

gcc ./src/*.c -o dicc -ggdb
