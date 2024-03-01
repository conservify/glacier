#

all: libpsn32.a AdcDemo

CXX ?= $(TOOLCHAIN_PREFIX)g++
AR ?= $(TOOLCHAIN_PREFIX)ar
ADC_CPPFLAGS = $(CPPFLAGS) -D__i386 -g -D_LINUX -D_INTEL -D_USE_SCHED  -D_USE_PTHREADS -D_USE_TERMIOS -O3 -Wall -Wno-write-strings -c $(CPPFLAGS_EXTRA)
ADC_LDFLAGS = $(LDFLAGS_EXTRA)

OBJS = PSNADBoard.o AdcBoard.o Adcv1.o Adcv2.o Adcvm.o AdcSdr24.o GpsRef.o GpsRefVco.o PcRef.o WwvRef.o WwvbRef.o
H = PSNADBoard.h Adcv1.h AdcNew.h AdcDemo.h

libpsn32.a: $(OBJS)
	rm -f libpsn32.a
	$(AR) -csq libpsn32.a $(OBJS)

.cpp.o:
	$(CXX) $(ADC_CPPFLAGS) $<

PSNADBoard.o: $(H)
AdcBoard.o: $(H)
Adcv1.o: $(H)
Adcv2.o: $(H)
Adcvm.o: $(H)
AdcSdr24.o: $(H)
GpsRef.o: $(H)
GpsRefVco.o: $(H)
PcRef.o: $(H)
WwvRef.o: $(H)
WwvbRef.o: $(H)

AdcDemo.o: AdcDemo.cpp
	$(CXX) $(ADC_CPPFLAGS) AdcDemo.cpp

AdcDemo: AdcDemo.o libpsn32.a AdcDemo.h
	$(CXX) -o AdcDemo AdcDemo.o -L. -lpsn32 -lpthread $(ADC_LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o
	rm -f libpsn32.a
	rm -f AdcDemo

# end
