#***********************************************************/
# File :  Makefile  (for  moldyn.c)
# Created  :  Shamik Sharma,
# Modified :  Mike Dai Wang 
#***********************************************************
CC = clang
CXX = clang++

ifeq ($(DEBUG), )
  DEBUG = -Wall -O3 #-DPRINT_COORDINATES
endif

#Application options
VERSION = -DSUPER
#    VERSION = -DVERSION_DYNAMIC
#    export VERSION

CXXFLAGS := $(DEBUG) $(VERSION) -std=c++11 -stdlib=libc++
LDFLAGS = -lm -lpthread -lrt

OUT_DIR = ..

.c.o:
	$(CXX) $(CXXFLAGS) -c $*.cc 

#***********************************************************
    TARGET = moldyn
    OBJS   = moldyn.o
#***********************************************************

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OUT_DIR)/$@ $(OBJS) $(LDFLAGS)

#***********************************************************
#    Dependencies
#***********************************************************

moldyn.o   : moldyn.cc Makefile

#***********************************************************
clean:
	/bin/rm -f $(OBJS) $(OUT_DIR)/$(TARGET)
