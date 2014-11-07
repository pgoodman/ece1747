#***********************************************************/
# File :  Makefile  (for  moldyn.c)
# Created  :  Shamik Sharma,
# Modified :  Mike Dai Wang 
#***********************************************************
CC = clang
CXX = clang++

#ifeq ($(DEBUG), )
#  DEBUG = -Wall -O3 #-DPRINT_COORDINATES
#endif

DEBUG = -O0 -g3

#Application options
VERSION = -DSUPER
#    VERSION = -DVERSION_DYNAMIC
#    export VERSION

CXXFLAGS := $(DEBUG) $(VERSION) -std=c++11 -stdlib=libc++
LDFLAGS = -lm -lpthread -lrt

OUT_DIR = .

all: moldyn.cc transaction.o pool.h
	$(CXX) $(CXXFLAGS) -o moldyn moldyn.cc transaction.o $(LDFLAGS)

transaction.o:  transaction.cc transaction.h
	$(CXX) $(CXXFLAGS) -c -o $@ transaction.cc

clean:
	rm -f transaction.o moldyn
	
