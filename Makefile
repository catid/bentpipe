# Change your compiler settings here

# Clang seems to produce faster code
#CCPP = g++
#CC = gcc
#OPTFLAGS = -O3 -fomit-frame-pointer -funroll-loops
CCPP = clang++ -m64
CC = clang -m64
OPTFLAGS = -O4
DBGFLAGS = -g -O0 -DDEBUG
CFLAGS = -Wall -fstrict-aliasing -I./libcat
LIBS =


# Object files

libcat_o = Sockets.o Enforcer.o

server_o = server.o $(libcat_o)


# Release target (default)

release : CFLAGS += $(OPTFLAGS)
release : server


# Debug target

debug : CFLAGS += $(DBGFLAGS)
debug : server


# Server target

server : clean $(server_o)
	$(CCPP) $(server_o) $(LIBS) -o bentpipe

valgrind : CFLAGS += -DUNIT_TEST $(DBGFLAGS)
valgrind : clean $(server_o)
	$(CCPP) $(server_o) $(LIBS) -o valgrindtest
	valgrind --dsymutil=yes --leak-check=yes ./valgrindtest


# Shared objects

Clock.o : libcat/Clock.cpp
	$(CCPP) $(CFLAGS) -c libcat/Clock.cpp

EndianNeutral.o : libcat/EndianNeutral.cpp
	$(CCPP) $(CFLAGS) -c libcat/EndianNeutral.cpp

SecureErase.o : libcat/SecureErase.cpp
	$(CCPP) $(CFLAGS) -c libcat/SecureErase.cpp

SecureEqual.o : libcat/SecureEqual.cpp
	$(CCPP) $(CFLAGS) -c libcat/SecureEqual.cpp

BitMath.o : libcat/BitMath.cpp
	$(CCPP) $(CFLAGS) -c libcat/BitMath.cpp

SipHash.o : libcat/SipHash.cpp
	$(CCPP) $(CFLAGS) -c libcat/SipHash.cpp

Sockets.o : libcat/Sockets.cpp
	$(CCPP) $(CFLAGS) -c libcat/Sockets.cpp

Enforcer.o : libcat/Enforcer.cpp
	$(CCPP) $(CFLAGS) -c libcat/Enforcer.cpp


# Executable objects

server.o : src/server.cpp
	$(CCPP) $(CFLAGS) -c src/server.cpp


# Cleanup

.PHONY : clean

clean :
	git submodule update --init
	-rm server valgrindtest *.o

