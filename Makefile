exe:
	gcc -o bmp-vyu bmp-vyu.c main.c -lraylib -lGL -lm -pthread -ldl -std=c99

lib:
	gcc -c -fPIC bmp-vyu.c -lraylib -lGL -lm -pthread -ldl -O3 -std=c99
	gcc -shared -o libbmp-vyu.so bmp-vyu.o
