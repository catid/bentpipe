# Change your compiler settings here

# Clang seems to produce faster code
#CCPP = g++
#CC = gcc
#OPTFLAGS = -O3 -fomit-frame-pointer -funroll-loops
CCPP = clang++ -m64
CC = clang -m64
OPTFLAGS = -O4
DBGFLAGS = -g -O0 -DDEBUG
CFLAGS = -Wall -fstrict-aliasing -I ./libcat -I ./shorthair/include -I ./shorthair/longhair/include
LIBS =


# Object files

libcat_o = Sockets.o Enforcer.o Clock.o EndianNeutral.o \
		 	ReuseAllocator.o BitMath.o MemXOR.o MemSwap.o

shorthair_o = Shorthair.o cauchy_256.o

server_o = server.o $(libcat_o) $(shorthair_o)


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

ReuseAllocator.o : libcat/ReuseAllocator.cpp
	$(CCPP) $(CFLAGS) -c libcat/ReuseAllocator.cpp

BitMath.o : libcat/BitMath.cpp
	$(CCPP) $(CFLAGS) -c libcat/BitMath.cpp

MemXOR.o : libcat/MemXOR.cpp
	$(CCPP) $(CFLAGS) -c libcat/MemXOR.cpp

MemSwap.o : libcat/MemSwap.cpp
	$(CCPP) $(CFLAGS) -c libcat/MemSwap.cpp


# Shorthair objects

cauchy_256.o : shorthair/longhair/src/cauchy_256.cpp
	$(CCPP) $(CFLAGS) -c shorthair/longhair/src/cauchy_256.cpp

Shorthair.o : shorthair/src/Shorthair.cpp
	$(CCPP) $(CFLAGS) -c shorthair/src/Shorthair.cpp


# Executable objects

server.o : src/server.cpp
	$(CCPP) $(CFLAGS) -c src/server.cpp


# Cleanup

.PHONY : clean

clean :
	git submodule update --init --recursive
	-rm server valgrindtest *.o

