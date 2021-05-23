$(CC) = gcc

all: pass-open-fd

pass-open-fd: pass-open-fd.c
	$(CC) -o pass-open-fd pass-open-fd.c
