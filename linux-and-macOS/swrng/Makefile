CC=gcc
GPP=g++
IDIR =./api-inc
SDIR =./api-src

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
	IDIR_MACOS = -I/opt/homebrew/opt/libusb/include/ -I/usr/local/include/
	LDIR_MACOS = -L/opt/homebrew/opt/libusb/lib/ -L/usr/local/lib/
	ifneq ($(wildcard /usr/local/opt/openssl/.),)
		OPENSSL_DIR_MACOS = /usr/local/opt/openssl
	else
		OPENSSL_DIR_MACOS = /opt/homebrew/opt/openssl
	endif
	OPENSSL_SUPPORT_INC_MACOS = -I$(OPENSSL_DIR_MACOS)/include
	OPENSSL_SUPPORT_LIB_MACOS = -L$(OPENSSL_DIR_MACOS)/lib
endif

CFLAGS = -O2 -I$(IDIR) $(IDIR_MACOS) -Wall -Wextra
CFLAGS_THREAD = -lpthread
CPPFLAGS = $(CFLAGS) -std=c++11
#CLANGSTD = -std=c99
LDFLAGS = -lusb-1.0 $(LDIR_MACOS)
LDCPPFLAGS = $(LDFLAGS) -lstdc++
CFLAGS_ENGINE= -I$(IDIR) $(IDIR_MACOS) $(OPENSSL_SUPPORT_INC_MACOS) -fPIC -Wall -std=c++11
LDFLAGS_ENGINE= -shared -lstdc++ -lusb-1.0 -lcrypto $(LDIR_MACOS) $(OPENSSL_SUPPORT_LIB_MACOS)

OBJECTS = SwiftRngApi.o USBSerialDevice.o SwiftRngApiCWrapper.o RandomSeqGenerator.o
ENG_API_SRCS = $(SDIR)/SwiftRngApi.cpp $(SDIR)/USBSerialDevice.cpp
CLOBJECTS = swrng-cl-api.o

SWDIAG = swdiag
SWPERFTEST = swperftest
SWPERFTEST_CL = swperf-cl-test
BITCOUNT = bitcount
BITCOUNT_CL = bitcount-cl
SWRNG = swrng
SWRNG_CL = swrng-cl
SWDIAG_CL = swdiag-cl
SWRAWRANDOM = swrawrandom
SWRNGSEQGEN = swrngseqgen
SAMPLE = sample
SAMPLECPP = sample++
SAMPLE_CL = sample-cl
SWRNG_ENGINE = eng_swiftrng

all: $(SAMPLE) $(SWDIAG) $(SWPERFTEST) $(BITCOUNT) $(SWRNG) $(SWRAWRANDOM) $(SWRNGSEQGEN) $(SAMPLE_CL) $(BITCOUNT_CL) $(SWDIAG_CL) $(SWPERFTEST_CL) $(SWRNG_CL) $(SAMPLECPP)

$(SAMPLE): $(SAMPLE).c $(OBJECTS)
	@echo
	@echo "Creating $(SAMPLE) ..."
	$(CC) -c $(SAMPLE).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SAMPLE).o $(OBJECTS) -o $(SAMPLE) $(LDFLAGS)

$(SAMPLECPP): $(SAMPLECPP).cpp $(OBJECTS)
	@echo
	@echo "Creating $(SAMPLECPP) ..."
	$(GPP) -c $(SAMPLECPP).cpp $(CPPFLAGS)
	$(GPP) $(SAMPLECPP).o $(OBJECTS) -o $(SAMPLECPP) $(LDCPPFLAGS)

$(SWDIAG): $(SWDIAG).c $(OBJECTS)
	@echo
	@echo "Creating $(SWDIAG) ..."
	$(CC) -c $(SWDIAG).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWDIAG).o $(OBJECTS) -o $(SWDIAG) $(LDFLAGS)

$(SWDIAG_CL): $(SWDIAG_CL).c $(OBJECTS) $(CLOBJECTS)
	@echo
	@echo "Creating $(SWDIAG_CL) ..."
	$(CC) -c $(SWDIAG_CL).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWDIAG_CL).o $(OBJECTS) $(CLOBJECTS) -o $(SWDIAG_CL) $(LDFLAGS) $(CFLAGS_THREAD)

$(SWPERFTEST): $(SWPERFTEST).c $(OBJECTS)
	@echo
	@echo "Creating $(SWPERFTEST) ..."
	$(CC) -c $(SWPERFTEST).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWPERFTEST).o $(OBJECTS) -o $(SWPERFTEST) $(LDFLAGS)

$(SWPERFTEST_CL): $(SWPERFTEST_CL).c $(OBJECTS) $(CLOBJECTS)
	@echo
	@echo "Creating $(SWPERFTEST_CL) ..."
	$(CC) -c $(SWPERFTEST_CL).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWPERFTEST_CL).o $(OBJECTS) $(CLOBJECTS) -o $(SWPERFTEST_CL) $(LDFLAGS) $(CFLAGS_THREAD)

$(BITCOUNT): $(BITCOUNT).c $(OBJECTS)
	@echo
	@echo "Creating $(BITCOUNT) ..."
	$(CC) -c $(BITCOUNT).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(BITCOUNT).o $(OBJECTS) -o $(BITCOUNT) $(LDFLAGS)

$(BITCOUNT_CL): $(BITCOUNT_CL).c $(OBJECTS) $(CLOBJECTS)
	@echo
	@echo "Creating $(BITCOUNT_CL) ..."
	$(CC) -c $(BITCOUNT_CL).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(BITCOUNT_CL).o $(OBJECTS) $(CLOBJECTS) -o $(BITCOUNT_CL) $(LDFLAGS) $(CFLAGS_THREAD)

$(SWRNG): $(SWRNG).c $(OBJECTS)
	@echo
	@echo "Creating $(SWRNG) ..."
	$(CC) -c $(SWRNG).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWRNG).o $(OBJECTS) -o $(SWRNG) $(LDFLAGS)

$(SWRNG_CL): $(SWRNG_CL).c $(OBJECTS) $(CLOBJECTS)
	@echo
	@echo "Creating $(SWRNG_CL) ..."
	$(CC) -c $(SWRNG_CL).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWRNG_CL).o $(OBJECTS) $(CLOBJECTS) -o $(SWRNG_CL) $(LDFLAGS) $(CFLAGS_THREAD)

$(SWRAWRANDOM): $(SWRAWRANDOM).c $(OBJECTS)
	@echo
	@echo "Creating $(SWRAWRANDOM) ..."
	$(CC) -c $(SWRAWRANDOM).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SWRAWRANDOM).o $(OBJECTS) -o $(SWRAWRANDOM) $(LDFLAGS)

$(SWRNGSEQGEN): $(SWRNGSEQGEN).cpp $(OBJECTS)
	@echo
	@echo "Creating $(SWRNGSEQGEN) ..."
	$(GPP) -c $(SWRNGSEQGEN).cpp $(CPPFLAGS)
	$(GPP) $(SWRNGSEQGEN).o $(OBJECTS) -o $(SWRNGSEQGEN) $(LDCPPFLAGS)

$(SAMPLE_CL): $(SAMPLE_CL).c $(OBJECTS) $(CLOBJECTS)
	@echo
	@echo "Creating $(SAMPLE_CL) ..."
	$(CC) -c $(SAMPLE_CL).c $(CFLAGS) $(CLANGSTD)
	$(GPP) $(SAMPLE_CL).o $(OBJECTS) $(CLOBJECTS) -o $(SAMPLE_CL) $(LDFLAGS) $(CFLAGS_THREAD)

$(SWRNG_ENGINE): $(SWRNG_ENGINE).cpp
	@echo
	@echo "Creating $(SWRNG_ENGINE) ..."
	$(CC) $(SWRNG_ENGINE).cpp $(ENG_API_SRCS) -o $(SWRNG_ENGINE).so $(CFLAGS_ENGINE) $(LDFLAGS_ENGINE)

SwiftRngApi.o:
	$(GPP) -c $(SDIR)/SwiftRngApi.cpp $(CPPFLAGS)
	
USBSerialDevice.o:
	$(GPP) -c $(SDIR)/USBSerialDevice.cpp $(CPPFLAGS)

SwiftRngApiCWrapper.o:
	$(GPP) -c $(SDIR)/SwiftRngApiCWrapper.cpp $(CPPFLAGS)

RandomSeqGenerator.o:
	$(GPP) -c $(SDIR)/RandomSeqGenerator.cpp $(CPPFLAGS)

swrng-cl-api.o:
	$(CC) -c $(SDIR)/swrng-cl-api.c $(CFLAGS)



clean:
	rm -f *.o ; rm -fr $(SAMPLE) $(SWDIAG) $(SWPERFTEST) $(BITCOUNT) $(SWRNG) $(SWRAWRANDOM) $(SWRNGSEQGEN) $(SAMPLE_CL) $(BITCOUNT_CL) $(SWDIAG_CL) $(SWPERFTEST_CL) $(SWRNG_CL) $(SAMPLECPP) $(SWRNG_ENGINE).so

install:
	install $(SWDIAG) $(BINDIR)/$(SWDIAG)
	install $(SWDIAG_CL) $(BINDIR)/$(SWDIAG_CL)
	install $(SWPERFTEST) $(BINDIR)/$(SWPERFTEST)
	install $(SWPERFTEST_CL) $(BINDIR)/$(SWPERFTEST_CL)
	install $(BITCOUNT) $(BINDIR)/$(BITCOUNT)
	install $(BITCOUNT_CL) $(BINDIR)/$(BITCOUNT_CL)
	install $(SWRNG) $(BINDIR)/$(SWRNG)
	install $(SWRNG_CL) $(BINDIR)/$(SWRNG_CL)
	install $(SWRAWRANDOM) $(BINDIR)/$(SWRAWRANDOM)
	install $(SWRNGSEQGEN) $(BINDIR)/$(SWRNGSEQGEN)

uninstall:
	rm $(BINDIR)/$(SWDIAG)
	rm $(BINDIR)/$(SWDIAG_CL)
	rm $(BINDIR)/$(SWPERFTEST)
	rm $(BINDIR)/$(SWPERFTEST_CL)
	rm $(BINDIR)/$(BITCOUNT)
	rm $(BINDIR)/$(BITCOUNT_CL)
	rm $(BINDIR)/$(SWRNG)
	rm $(BINDIR)/$(SWRNG_CL)
	rm $(BINDIR)/$(SWRAWRANDOM)
	rm $(BINDIR)/$(SWRNGSEQGEN)
	

