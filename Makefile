CC=gcc
PROJECT=hyprp
CFLAGS= -Wall \
		-Werror \
		-std=c99 \
		-ljq

.PHONY: clean

all: $(PROJECT)

$(PROJECT): build/$(PROJECT).o
	ar -rc build/lib$@.a build/*.o

build/%.o: src/%.c include/%.h
	$(CC) $(CFLAGS)	-o $@ -c $<

clean:
	rm -rf build/*
