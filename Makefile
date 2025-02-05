exe:
	gcc -o bmp-vyu bmp-vyu.c main.c -lraylib -lGL -lm -pthread -ldl

lib:
	gcc -c -fpic bmp-vyu.c lib.c -lraylib -lGL -lm -pthread -ldl
	gcc -shared -o libbmp-vyu.so bmp-vyu.o
