CC=gcc
SWRNG_FLAGS=-D_FILE_OFFSET_BITS=64
CFLAGS= -lusb-1.0 -O2 -Wall -Wextra
CPPFLAGS=-lstdc++
CFLAGS_THREAD=-lpthread

PREFIX = $(DESTDIR)/usr/local
BINDIR = $(PREFIX)/bin

OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
CFLAGS += -L/usr/local/lib/ -I /usr/local/include/ 
endif

SWRNG = swrng
SWRNG_CL = swrng-cl
SAMPLE_PROGR = sample
SAMPLE_CL_PROGR = sample-cl
SWDIAG = swdiag
SWDIAG_CL = swdiag-cl
SWPERFTEST = swperftest
SWPERF_CL_TEST = swperf-cl-test
BITCOUNT = bitcount
BITCOUNT_CL = bitcount-cl
SWRAWRANDOM = swrawrandom
SWRNGSEQGEN = swrngseqgen
SWRNG_PSERVER = run-swrng-pserver.sh

all: $(SWRNG) $(SWRNG_CL) $(SAMPLE_PROGR) $(SWDIAG) $(SWDIAG_CL) $(SWPERFTEST) $(SAMPLE_CL_PROGR) $(SWPERF_CL_TEST) $(BITCOUNT) $(BITCOUNT_CL) $(SWRNGSEQGEN) $(SWRAWRANDOM)

$(SWRNG): swrng.cpp
	$(CC) swrng.cpp swrngapi.cpp USBSerialDevice.cpp -o $(SWRNG) $(CFLAGS) $(CPPFLAGS)

$(SWRNG_CL): swrng-cl.cpp
	$(CC) swrng-cl.cpp swrngapi.cpp USBSerialDevice.cpp swrng-cl-api.cpp -o $(SWRNG_CL) $(CFLAGS) $(CFLAGS_THREAD) $(CPPFLAGS)

$(SAMPLE_PROGR): sample.cpp
	$(CC) sample.cpp swrngapi.cpp USBSerialDevice.cpp -o $(SAMPLE_PROGR) $(CFLAGS) $(CPPFLAGS)

$(SAMPLE_CL_PROGR): sample-cl.cpp
	$(CC) sample-cl.cpp swrngapi.cpp USBSerialDevice.cpp swrng-cl-api.cpp -o $(SAMPLE_CL_PROGR) $(CFLAGS) $(CFLAGS_THREAD) $(CPPFLAGS)

$(SWPERFTEST): swperftest.cpp
	$(CC) swperftest.cpp swrngapi.cpp USBSerialDevice.cpp -o $(SWPERFTEST) $(CFLAGS) $(CPPFLAGS)

$(SWPERF_CL_TEST): swperf-cl-test.cpp
	$(CC) swperf-cl-test.cpp swrngapi.cpp USBSerialDevice.cpp swrng-cl-api.cpp -o $(SWPERF_CL_TEST) $(CFLAGS) $(CFLAGS_THREAD) $(CPPFLAGS)

$(SWDIAG): swdiag.cpp
	$(CC) swdiag.cpp swrngapi.cpp USBSerialDevice.cpp -o $(SWDIAG) $(CFLAGS) -lm $(CPPFLAGS)

$(SWDIAG_CL): swdiag-cl.cpp
	$(CC) swdiag-cl.cpp swrngapi.cpp USBSerialDevice.cpp swrng-cl-api.cpp -o $(SWDIAG_CL) $(CFLAGS) $(CFLAGS_THREAD) -lm $(CPPFLAGS)

$(BITCOUNT): bitcount.cpp
	$(CC) bitcount.cpp swrngapi.cpp USBSerialDevice.cpp -o $(BITCOUNT) $(CFLAGS) $(CPPFLAGS)

$(BITCOUNT_CL): bitcount-cl.cpp
	$(CC) bitcount-cl.cpp swrngapi.cpp USBSerialDevice.cpp swrng-cl-api.cpp -o $(BITCOUNT_CL) $(CFLAGS) $(CFLAGS_THREAD) $(CPPFLAGS)

$(SWRAWRANDOM): swrawrandom.cpp
	$(CC) swrawrandom.cpp swrngapi.cpp USBSerialDevice.cpp -o $(SWRAWRANDOM) $(CFLAGS) $(CPPFLAGS)

$(SWRNGSEQGEN): swrngseqgen.cpp
	$(CC) swrngseqgen.cpp swrngapi.cpp USBSerialDevice.cpp SWRNGRandomSeqGenerator.cpp -o $(SWRNGSEQGEN) $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f *.o ; rm $(SWRNG) $(SWRNG_CL) $(SAMPLE_PROGR) $(SWDIAG) $(SWDIAG_CL) $(SWPERFTEST) $(SAMPLE_CL_PROGR) $(SWPERF_CL_TEST) $(BITCOUNT) $(BITCOUNT_CL) $(SWRNGSEQGEN) $(SWRAWRANDOM)

install:
	install $(SWRNG) $(BINDIR)/$(SWRNG)
	install $(SWRNG_CL) $(BINDIR)/$(SWRNG_CL)
	install $(SWDIAG) $(BINDIR)/$(SWDIAG)
	install $(SWDIAG_CL) $(BINDIR)/$(SWDIAG_CL)
	install $(SWPERFTEST) $(BINDIR)/$(SWPERFTEST)
	install $(SWPERF_CL_TEST) $(BINDIR)/$(SWPERF_CL_TEST)
	install $(SWRNGSEQGEN) $(BINDIR)/$(SWRNGSEQGEN)
	install $(BITCOUNT) $(BINDIR)/$(BITCOUNT)
	install $(BITCOUNT_CL) $(BINDIR)/$(BITCOUNT_CL)
	install $(SWRAWRANDOM) $(BINDIR)/$(SWRAWRANDOM)
	cp $(SWRNG_PSERVER) $(BINDIR)/$(SWRNG_PSERVER)
	chmod a+x $(BINDIR)/$(SWRNG_PSERVER)

uninstall:
	rm $(BINDIR)/$(SWRNG)
	rm $(BINDIR)/$(SWRNG_CL)
	rm $(BINDIR)/$(SWDIAG)
	rm $(BINDIR)/$(SWDIAG_CL)
	rm $(BINDIR)/$(SWPERFTEST)
	rm $(BINDIR)/$(SWPERF_CL_TEST)
	rm $(BINDIR)/$(SWRNGSEQGEN)
	rm $(BINDIR)/$(BITCOUNT)
	rm $(BINDIR)/$(BITCOUNT_CL)
	rm $(BINDIR)/$(SWRAWRANDOM)
	rm $(BINDIR)/$(SWRNG_PSERVER)
