
CC= gcc
CFLAGS= -Wall -Wextra -g -O0

make:
	$(CC) $(CFLAGS) main.c -o main
