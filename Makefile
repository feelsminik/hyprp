CC=gcc
PROJECT=hyprp
CFLAGS= -Wall 				\
		-std=c99 			\
		-D_XOPEN_SOURCE=700
LIBS= -lhyprp \
	  -Lbuild

.PHONY: clean build/hyprp-test test

all: build/hyprp-test

build/hyprp-test: build/hyprp-test.o build/libhyprp.a
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

build/hyprp-test.o: test/hyprp-test.c include/hyprp.h
	$(CC) $(CFLAGS) -o $@ -c $<

build/libhyprp.a: build/hyprp.o
	ar -rcs $@ $^

build/hyprp.o: src/hyprp.c include/hyprp.h
	$(CC) $(CFLAGS) -o $@ -c $<

test: build/hyprp-test
	./build/hyprp-test

clean:
	rm -rf build/*
