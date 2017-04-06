CC=g++
CFLAGS=-std=c++11 -Wall -I.

DEPS = sockethandler.h \

OBJS = client.o \
	   sockethandler.o \


%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: client

debug: CFLAGS += -DDEBUG_BUILD
debug: all

client: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@


clean:
	rm *.o client 
