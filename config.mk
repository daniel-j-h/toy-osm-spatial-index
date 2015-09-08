CXX = g++
CC = gcc

CXXFLAGS += -pthread -fdiagnostics-color=auto -fmax-errors=1 -std=c++14 -O2 -Wall -Wextra -pedantic -Wuninitialized -Wstrict-overflow=3 -ffunction-sections -fdata-sections
CXXFLAGS += -I./libs/libosmium-2.2.0/include -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_FORTIFY_SOURCE=2

LDFLAGS += -Wl,-O1 -Wl,--hash-style=gnu -Wl,--sort-common -Wl,--build-id -Wl,--gc-sections
LDLIBS += -lstdc++ -lm -lpthread -lprotobuf-lite -losmpbf -lz -lboost_timer -lboost_system -lboost_chrono -ltbb -ltbbmalloc -ltbbmalloc -ltbbmalloc_proxy
