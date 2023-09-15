CC=gcc
CFLAGS=
SRC=src/main.c
OUTPUT=bin/lsh
#CFLAGS=-DLSH_USE_STD_GETLINE

all: build

build: folders
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT)

folders:
	@mkdir -pv src bin

clean:
	@rm -vf $(OUTPUT)
	@if [ -d bin ]; then rmdir --ignore-fail-on-non-empty bin; fi

$(OUTPUT): build

run: $(OUTPUT)
	$(OUTPUT)

.PHONY: all build clean run
