# You can put your build options here
-include config.mk

parallel: main.c jasmine.c
	$(CC) -g -c jasmine.c -DJSMN_PARALLEL
	$(CC) -g -c main.c -DJSMN_PARA_THR=1 -DJSMN_PARALLEL
	$(CC) -g -o main2 main.o jasmine.o -pthread


test: test_default test_strict test_links test_strict_links
test_default: test/tests.c jsmn.h
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o test/$@
	./test/$@
test_strict: test/tests.c jsmn.h
	$(CC) -DJSMN_STRICT=1 $(CFLAGS) $(LDFLAGS) $< -o test/$@
	./test/$@
test_links: test/tests.c jsmn.h
	$(CC) -DJSMN_PARENT_LINKS=1 $(CFLAGS) $(LDFLAGS) $< -o test/$@
	./test/$@
test_strict_links: test/tests.c jsmn.h
	$(CC) -DJSMN_STRICT=1 -DJSMN_PARENT_LINKS=1 $(CFLAGS) $(LDFLAGS) $< -o test/$@
	./test/$@

simple_example: example/simple.c jsmn.h
	$(CC) $(LDFLAGS) $< -o $@

jsondump: example/jsondump.c jsmn.h
	$(CC) $(LDFLAGS) $< -o $@

fmt:
	clang-format -i jsmn.h test/*.[ch] example/*.[ch]

lint:
	clang-tidy jsmn.h --checks='*'

clean:
	rm -f *.o example/*.o
	rm -f simple_example
	rm -f jsondump
	rm -f test/test_*
	rm -f main main2

.PHONY: clean test

