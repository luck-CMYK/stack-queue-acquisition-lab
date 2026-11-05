CC = gcc
CFLAGS = -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow -I.
DS = ds/cqueue.c ds/seqstack.c ds/liststack.c ds/listqueue.c
HEADERS = $(wildcard ds/*.h) app/expression.h app/acquisition.h
# Windows entry points redirect all temporary files to this workspace.
export TEMP := $(CURDIR)/build/tmp
export TMP := $(CURDIR)/build/tmp
export TMPDIR := $(CURDIR)/build/tmp

.PHONY: all test verify bench
all: build/lab.exe build/benchmark.exe
build/lab.exe: $(DS) app/acquisition.c app/expression.c app/main.c $(HEADERS) | build/tmp
	$(CC) $(CFLAGS) $(DS) app/acquisition.c app/expression.c app/main.c -o $@
build/benchmark.exe: $(DS) bench/benchmark.c $(HEADERS) | build/tmp
	$(CC) $(CFLAGS) $(DS) bench/benchmark.c -o $@
build/tmp:
	powershell -NoProfile -Command "New-Item -ItemType Directory -Path build/tmp -Force | Out-Null"
test:
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test.ps1
verify:
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify.ps1
bench:
	powershell -NoProfile -ExecutionPolicy Bypass -File scripts/benchmark.ps1
