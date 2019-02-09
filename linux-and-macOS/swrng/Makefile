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

$(SWRNG): swrng.c
	$(CC) swrng.c swrngapi.c -o $(SWRNG) $(CFLAGS)

$(SWRNG_CL): swrng-cl.c
	$(CC) swrng-cl.c swrngapi.c swrng-cl-api.c -o $(SWRNG_CL) $(CFLAGS) $(CFLAGS_THREAD)

$(SAMPLE_PROGR): sample.c
	$(CC) sample.c swrngapi.c -o $(SAMPLE_PROGR) $(CFLAGS)

$(SAMPLE_CL_PROGR): sample-cl.c
	$(CC) sample-cl.c swrngapi.c swrng-cl-api.c -o $(SAMPLE_CL_PROGR) $(CFLAGS) $(CFLAGS_THREAD)

$(SWPERFTEST): swperftest.c
	$(CC) swperftest.c swrngapi.c -o $(SWPERFTEST) $(CFLAGS)

$(SWPERF_CL_TEST): swperf-cl-test.c
	$(CC) swperf-cl-test.c swrngapi.c swrng-cl-api.c -o $(SWPERF_CL_TEST) $(CFLAGS) $(CFLAGS_THREAD)

$(SWDIAG): swdiag.c
	$(CC) swdiag.c swrngapi.c -o $(SWDIAG) $(CFLAGS) -lm

$(SWDIAG_CL): swdiag-cl.c
	$(CC) swdiag-cl.c swrngapi.c swrng-cl-api.c -o $(SWDIAG_CL) $(CFLAGS) $(CFLAGS_THREAD) -lm

$(BITCOUNT): bitcount.c
	$(CC) bitcount.c swrngapi.c -o $(BITCOUNT) $(CFLAGS)

$(BITCOUNT_CL): bitcount-cl.c
	$(CC) bitcount-cl.c swrngapi.c swrng-cl-api.c -o $(BITCOUNT_CL) $(CFLAGS) $(CFLAGS_THREAD)

$(SWRAWRANDOM): swrawrandom.c
	$(CC) swrawrandom.c swrngapi.c -o $(SWRAWRANDOM) $(CFLAGS)

$(SWRNGSEQGEN): swrngseqgen.cpp
	$(CC) swrngseqgen.cpp swrngapi.c SWRNGRandomSeqGenerator.cpp -o $(SWRNGSEQGEN) $(CFLAGS) $(CPPFLAGS)

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
