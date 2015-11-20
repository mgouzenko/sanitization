CC=g++
SRCEXT=cpp
SRCS=$(shell find . -type f -name '*.$(SRCEXT)')
OBJS=$(subst .$(SRCEXT),.o,$(SRCS))
TESTS=$(shell find . -type f -name '*_test.sh')
CFLAGS := -g -Wall -std=c++0x

build:	add_file

add_files: $(OBJS)

%.o: %.$(SRCEXT)
	$(CC) $(CFLAGS) -c -o $@ $<

exec: build
	./add_file $(ARG)

clean:
	rm -f add_file *.core *.o
