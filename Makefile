all: build lib

VERSION=0.3

OBJ = ov_types errmsg common udpsocket callerlist ovboxclient

BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))

CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++11 -pthread	\
-ggdb -fno-finite-math-only -fext-numeric-literals

FULLVERSION=$(VERSION)

CXXFLAGS += -DOVBOXVERSION="\"$(FULLVERSION)\""

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

CPPFLAGS = -std=c++11
BUILD_DIR = build
SOURCE_DIR = src

lib: build/libov.a

build/libov.a: $(BUILD_OBJ)
	ar -rcs $@ $^

build: build/.directory

%/.directory:
	mkdir -p $*
	touch $@

$(BUILD_OBJ): $(wildcard src/*.h)

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -c $^ -o $@

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~
