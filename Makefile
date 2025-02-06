exe:
	gcc -o bmp-vyu bmp-vyu.c main.c -lraylib -lGL -lm -pthread -ldl -std=c99

lib:
	gcc -shared -o libbmp-vyu.so -fPIC bmp-vyu.c -lraylib -lGL -lm -pthread -ldl -O3 -std=c99
