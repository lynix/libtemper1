CFLAGS  = -Wall -O2 -g -std=gnu99 -pedantic
PROGN	= libtemper1

.PHONY: all clean


all: lib/$(PROGN).a

lib/$(PROGN).a: obj/$(PROGN).o
	ar rcs $@ $+
	
obj/$(PROGN).o: src/$(PROGN).c include/$(PROGN).h
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f lib/$(PROGN).a obj/*.o
