
# Author   Martin Dubois, P.Eng.
# Product  TinyHTTP
# File     makefile

OUTPUT = ../Binaries/TinyHTTP

SOURCES = TinyHTTP.cpp

# ===== Rules ===============================================================

.cpp.o:
	g++ -c $(CFLAGS) -o $@ $<

# ===== Macros ==============================================================

OBJECTS = $(SOURCES:.cpp=.o)

# ===== Targets =============================================================

$(OUTPUT) : $(OBJECTS)
	g++ -o $@ $^
	sudo setcap CAP_NET_BIND_SERVICE=+eip $(OUTPUT)

depend:
	makedepend $(SOURCES)

# DO NOT DELETE

TinyHTTP.o: /usr/include/stdio.h
