####################################################################################################
##
## CONFIGURATION
##
####################################################################################################

## Compilation parameters
CC = clang
CXX = clang++

ifeq ($(DEBUG), )
	DEBUG = -O3
endif

CXXFLAGS := -Wall $(DEBUG) $(VERSION) $(OPTIONS) -std=c++11 -stdlib=libc++ -D_GNU_SOURCE
LDFLAGS = -lpthread


## Source and binary files
BIN_DIR   =   build
SRC_DIR   =   moldyn_src
APP_DIR   =   moldyn_src

C_SRCS    =  # $(wildcard $(SRC_DIR)/*.c)
C_OBJS    =   $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(C_SRCS)) 
CC_SRCS   =   $(wildcard $(SRC_DIR)/*.cc)
CC_OBJS   =   $(patsubst $(SRC_DIR)/%.cc,$(BIN_DIR)/%.o,$(CC_SRCS)) 


all: init Makefile

init:
					mkdir -p $(BIN_DIR)
					@echo $(C_OBJS)
					@echo $(CC_OBJS)

# For C source files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c Makefile
	${CXX} ${CXXFLAGS} -o $@ -c $<

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cc Makefile
	${CXX} ${CXXFLAGS} -o $@ -c $<

default: all 

moldyn:	Makefile
						$(MAKE) -C moldyn_src moldyn

clean:
		-rm -f -R $(BIN_DIR)
		-rm -f 

clean_moldyn:
		$(MAKE) -C apps/moldyn clean

clean_all:	clean clean_moldyn

.PHONY: all clean
