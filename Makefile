CFLAGS = -std=gnu99
LIBS =
SOURCES = project3.c 3cr.c
OUT = out
default:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
debug:
	gcc -g $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
all:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
clean:
	rm $(OUT)
