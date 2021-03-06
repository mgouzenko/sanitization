CC=clang++
SRCEXT=cpp
SRCS=$(shell find . -type f -name '*.$(SRCEXT)')
OBJS=$(subst .$(SRCEXT),.o,$(SRCS))
TEST_INPUTS=$(shell find . -type f -name 'test_*.in')
CFLAGS := -g -Wall -std=c++11

build:	add_file

add_file: $(OBJS)

%.o: %.$(SRCEXT)
	$(CC) $(CFLAGS) -c -o $@ $<

exec: build
	./add_file


.PHONY: .FORCE clean_test_root

clean_test_root:
	@rm -rf ./test_root/*

%.in: .FORCE
	@echo -n "================= $@ "
	@python -c "print('=' * (40 - len('$@')))"
	@mkdir test_root/$(patsubst tests/%.in,%,$@)
	@cd test_root/$(patsubst tests/%.in,%,$@) && ../../add_file < ../../$@
	@cd ../../
	@echo "Testing filename(s) against expected output"
	@ls -A test_root/$(patsubst tests/%.in,%,$@) | od -c | diff - $(subst .in,.out.file, $@) || true
	@echo "Testing data against expected output"
	@bash -c 'GLOBIGNORE=".:.."; shopt -s dotglob; cat test_root/$(patsubst tests/%.in,%,$@)/* 2>&1 | od -c | diff - $(subst .in,.out.data, $@) || true'
	@echo ===========================================================
	@echo

test: add_file clean_test_root $(TEST_INPUTS)
# @rm -rf ./test_root/*
# cd test_root && ../add_file < ../test_inputs.txt

clean: clean_test_root
	rm -f add_file *.core *.o
