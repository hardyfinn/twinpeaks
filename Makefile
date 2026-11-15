# Path to the Rack SDK (override on the command line: make RACK_DIR=...)
RACK_DIR ?= ../Rack-SDK

FLAGS +=
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

ifneq ($(MAKECMDGOALS),test)
include $(RACK_DIR)/plugin.mk
endif

# Offline engine checks + demo WAV (no Rack needed): make test
test/render: test/render.cpp src/TwinPeaksDSP.hpp
	$(CXX) -O2 -std=c++17 test/render.cpp -o test/render
.PHONY: test
test: test/render
	./test/render .
