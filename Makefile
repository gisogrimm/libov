VERSION=0.30
# keep the version number always in the first line of the Makefile!

# no c++2a or c++20, max 17 for the time being (c++-2a breaks build on
# CI pipeline for Ubuntu and arm):
CXXFLAGS = -Wall -Wno-deprecated-declarations -std=c++17 -pthread	\
-ggdb -fno-finite-math-only -fPIC -Wno-psabi -Wconversion -Wextra -Wno-unused-parameter

# external libraries used in the ovbox:
EXTERNALS = jack xerces-c liblo sndfile libcurl gsl samplerate fftw3f xerces-c libsodium

all: tascarplugins lib
cli: tascarpluginscli lib

export FULLVERSION:=$(shell ./get_version.sh)

BASEOBJ = ov_types errmsg common udpsocket ovtcpsocket callerlist	\
	ov_tools MACAddressUtility

OBJ = $(BASEOBJ) ovboxclient ov_client_orlandoviols	\
  ov_render_tascar soundcardtools

HAS_LSL:=$(shell tascar/check_for_lsl)

BUILD_OBJ = $(patsubst %,build/%.o,$(OBJ))

BUILD_OBJ_SERVER = $(patsubst %,build/%.o,$(BASEOBJ))

CXXFLAGS += -DOVBOXVERSION="\"$(FULLVERSION)\""

ifeq "$(ARCH)" "x86_64"
CXXFLAGS += -msse -msse2 -mfpmath=sse -ffast-math
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
LIBVAR=LD_LIBRARY_PATH
endif
ifeq ($(UNAME_S),Darwin)
LIBVAR=DYLD_LIBRARY_PATH
endif

#CPPFLAGS = -std=c++2a
PREFIX = /usr/local
BUILD_DIR = build
SOURCE_DIR = src

LDLIBS += `pkg-config --libs $(EXTERNALS)`
CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += -ldl -lovclienttascar
LDFLAGS += -Ltascar/libtascar/build

HEADER := $(wildcard src/*.h) $(wildcard libov/src/*.h) build/tscver

CXXFLAGS += -Itascar/libtascar/build

TASCAROBJECTS = licensehandler.o audiostates.o coordinates.o		\
  audiochunks.o tscconfig.o dynamicobjects.o sourcemod.o		\
  receivermod.o filterclass.o osc_helper.o pluginprocessor.o		\
  acousticmodel.o scene.o render.o session_reader.o session.o		\
  jackclient.o delayline.o errorhandling.o osc_scene.o ringbuffer.o	\
  jackiowav.o jackrender.o audioplugin.o levelmeter.o serviceclass.o	\
  speakerarray.o spectrum.o fft.o stft.o ola.o tascar_os.o		\
  spawn_process.o tictoctimer.o

TASCARDMXOBJECTS =

TASCARRECEIVERS = ortf hrtf simplefdn omni itu51

TASCARSOURCE = omni cardioidmod

TASCARMODULS = system touchosc waitforjackport route jackrec sleep	\
  epicycles hossustain hoafdnrot matrix savegains granularsynth		\
  oscserver oscrelay pos2osc echoc oscheadtracker osceog systime	\
  pos2osc oscactor snapangle

ifeq ($(UNAME_S),Linux)
TASCAROBJECTS += alsamidicc.o
TASCARMODULS += ltcgen midictl midicc2osc mididispatch serialheadtracker
endif
ifeq ($(UNAME_S),Darwin)
TASCARMODULS += ltcgen serialheadtracker
endif

ifeq "$(HAS_LSL)" "yes"
TASCARMODULS += lsl2osc osc2lsl lslactor
endif

TASCARMODULSGUI = tracegui datalogging

TASCARAUDIOPLUGS = sndfile delay metronome bandpass filter	\
  loopmachine spksim tubesim feedbackdelay gain sine pink noise	\
  flanger ringmodulator level2osc level2hsv bandlevel2osc	\
  reclevelanalyzer pareq4band

OSFLAG :=
ifeq ($(OS),Windows_NT)
	OSFLAG += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OSFLAG += -D AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAG += -D IA32
	endif
	EXTERNALS += portaudio-2.0
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		DISTRO = $(shell . /etc/os-release && echo $${ID})
		OSFLAG += -D LINUX
		CXXFLAGS += -fext-numeric-literals
		LDLIBS += -lasound
		TASCARMODULS += ovheadtracker lightctl midicc2osc
		TASCARDMXOBJECTS += termsetbaud.o serialport.o dmxdriver.o
		TASCARAUDIOPLUGS += simplesynth
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX -D DARWIN
		LDFLAGS += -framework IOKit -framework CoreFoundation
		CXXFLAGS += -I`brew --prefix libsoundio`/include
		LDFLAGS += -L`brew --prefix libsoundio`/lib
		LDLIBS += -lsoundio
		LDLIBS += `pkg-config --libs nlohmann_json`
		CXXFLAGS += `pkg-config --cflags nlohmann_json`
		ifeq ($(ARCH),arm64)
			CXXFLAGS += "--target=arm64-apple-darwin"
		endif
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

CXXFLAGS += $(OSFLAG)

showver: tascar/Makefile get_version.sh
	@echo $(FULLVERSION)

dist:
	echo $(DISTRO)

tascar/Makefile:
	git submodule init && git submodule update

lib: build build/libov.a

libovserver: EXTERNALS=libcurl xerces-c libsodium
libovserver: build/tscobj build/libovserver.a

$(BUILD_OBJ): build/tscobjcli

build/libov.a: $(BUILD_OBJ)
	ar rcs $@ $^

build/libovserver.a: $(patsubst %,tascar/libtascar/build/%,$(TASCAROBJECTS)) $(BUILD_OBJ_SERVER)
	ar rcs $@ $^

build: build/.directory

%/.directory:
	mkdir -p $* && touch $@

build/%.o: src/%.cc $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -c $< -o $@

##
## TASCAR related targets:
##

# create build directory within TASCAR:
build/tscbuild:
	$(MAKE) -C tascar/libtascar build && mkdir -p build && touch $@

# build SOFA and LSL library within TASCAR:
build/tscsofa: build
	$(MAKE) -C tascar libmysofa liblsl && touch $@

# generate TASCAR version number and related header files:
build/tscver: build/tscbuild build/tscsofa
	$(MAKE) -C tascar/libtascar ver && mkdir -p build && touch $@

# build TASCAR library with GUI components:
build/tscobj: build/tscobjcli
	$(MAKE) -C tascar/libtascar PLUGINPREFIX=ovclient TSCCXXFLAGS=-DPLUGINPREFIX='\"ovclient\"' all && mkdir -p build && touch $@

# build TASCAR library without GUI components:
build/tscobjcli: build/tscver
	$(MAKE) -C tascar/libtascar PLUGINPREFIX=ovclient TSCCXXFLAGS=-DPLUGINPREFIX='\"ovclient\"' cli && mkdir -p build && touch $@

# build TASCAR plugins, including GUI plugins:
tascarplugins: build/tscobj
	 $(MAKE) -C tascar/plugins PLUGINPREFIX=ovclient RECEIVERS="$(TASCARRECEIVERS)" SOURCES="$(TASCARSOURCE)" TASCARMODS="$(TASCARMODULS)" TASCARMODSGUI="$(TASCARMODULSGUI)" AUDIOPLUGINS="$(TASCARAUDIOPLUGS)" GLABSENSORS= TASCARLIB="-lovclienttascar" TASCARGUILIB="-lovclienttascargui" TASCARDMXLIB="-lovclienttascardmx"

# build TASCAR plugins, only non-GUI:
tascarpluginscli: build/tscobjcli
	 $(MAKE) -C tascar/plugins PLUGINPREFIX=ovclient RECEIVERS="$(TASCARRECEIVERS)" SOURCES="$(TASCARSOURCE)" TASCARMODS="$(TASCARMODULS)" TASCARMODSGUI="" AUDIOPLUGINS="$(TASCARAUDIOPLUGS)" GLABSENSORS= TASCARLIB="-lovclienttascar" TASCARGUILIB="" TASCARDMXLIB="-lovclienttascardmx"

##
## general purpose targets:
##

clangformat:
	clang-format-9 -i $(wildcard src/*.cc) $(wildcard src/*.h)

clean:
	rm -Rf build src/*~ .ov_version .ov_minor_version .ov_full_version .ov_commitver libtascar googletest CMakeFiles
	-$(MAKE) -C tascar clean


##
## unit testing:
##

googletest/WORKSPACE:
	git clone https://github.com/google/googletest &&	(cd googletest && git checkout v1.16.0)

gtest:
	$(MAKE) googlemock

googlemock: $(BUILD_DIR)/googlemock.is_installed

$(BUILD_DIR)/googlemock.is_installed: googletest/WORKSPACE \
	$(BUILD_DIR)/lib/.directory $(BUILD_DIR)/include/.directory
	echo $(CXXFLAGS)
	cd googletest/ && cmake ./ && $(MAKE)
	cp googletest/lib/*.a $(BUILD_DIR)/lib/
	cp -a googletest/googletest/include/gtest $(BUILD_DIR)/include/
	cp -a googletest/googlemock/include/gmock $(BUILD_DIR)/include/
	touch $@

unit-tests: lib gtest execute-unit-tests $(patsubst %,%-subdir-unit-tests,$(SUBDIRS))

$(patsubst %,%-subdir-unit-tests,$(SUBDIRS)):
	$(MAKE) -C $(@:-subdir-unit-tests=) unit-tests

execute-unit-tests: $(BUILD_DIR)/unit-test-runner
	if [ -x $< ]; then $(LIBVAR)=./build:./tascar/libtascar/build: $<; fi


unit_tests_test_files = $(wildcard unittests/*.cc)
$(BUILD_DIR)/unit-test-runner: $(BUILD_DIR)/.directory $(unit_tests_test_files) $(BUILD_OBJ)
	$(MAKE) gtest && if test -n "$(unit_tests_test_files)"; then $(CXX) $(CXXFLAGS) -I$(BUILD_DIR)/include -I$(SOURCE_DIR) -L$(BUILD_DIR) -L$(BUILD_DIR)/lib -o $@ $(wordlist 2, $(words $^), $^)  $(LDFLAGS) -lov $(LDLIBS) -lgtest_main -lgtest -lpthread; fi
