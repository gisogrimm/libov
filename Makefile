all: build lib

CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only

OSFLAG :=
ifeq ($(OS),Windows_NT)
	OSFLAG += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OSFLAG += -D AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAG += -D IA32
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
		CXXFLAGS += -fext-numeric-literals
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
	endif
		UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		OSFLAG += -D AMD64
	endif
		ifneq ($(filter %86,$(UNAME_P)),)
	OSFLAG += -D IA32
		endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		OSFLAG += -D ARM
	endif
endif

VERSION=0.3

OBJ = ov_types errmsg common udpsocket callerlist ovboxclient

BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))

FULLVERSION=$(VERSION)

CXXFLAGS += -DOVBOXVERSION="\"$(FULLVERSION)\"" $(OSFLAG)

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++11
BUILD_DIR = build
SOURCE_DIR = src

lib: build/libov.a

build/libov.a: $(BUILD_OBJ)
	ar rcs $@ $^

build: build/.directory

%/.directory:
	mkdir -p $*
	touch $@

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~
