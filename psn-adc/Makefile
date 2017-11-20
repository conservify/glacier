#

all: libpsn32.a AdcDemo

# TOOLCHAIN_PREFIX=
# CPPFLAGS_EXTRA=
# LDFLAGS_EXTRA=
GXX_COMPILER=$(TOOLCHAIN_PREFIX)g++
AR_COMPILER=$(TOOLCHAIN_PREFIX)ar
CPPFLAGS= -D__i386 -D_LINUX -D_INTEL -D_USE_SCHED  -D_USE_PTHREADS -D_USE_TERMIOS -O3 -Wall -Wno-write-strings -c $(CPPFLAGS_EXTRA)
LDFLAGS=$(LDFLAGS_EXTRA)
DOCKER_IMAGE_VERSION=latest
DOCKER_IMAGE_NAME=conservify/adc-logger
DOCKER_IMAGE_TAGNAME=$(DOCKER_IMAGE_NAME):$(DOCKER_IMAGE_VERSION)

OBJS= PSNADBoard.o AdcBoard.o Adcv1.o Adcv2.o Adcvm.o AdcSdr24.o GpsRef.o GpsRefVco.o PcRef.o WwvRef.o WwvbRef.o
H= PSNADBoard.h Adcv1.h AdcNew.h AdcDemo.h

libpsn32.a : $(OBJS)
	rm -f libpsn32.a
	$(AR_COMPILER) -csq libpsn32.a $(OBJS)

.cpp.o:
	$(GXX_COMPILER)  $(CPPFLAGS) $<

PSNADBoard.o : $(H)
AdcBoard.o : $(H)
Adcv1.o : $(H)
Adcv2.o : $(H)
Adcvm.o : $(H)
AdcSdr24.o : $(H) 
GpsRef.o : $(H) 
GpsRefVco.o : $(H)
PcRef.o : $(H)
WwvRef.o : $(H)
WwvbRef.o : $(H)

AdcDemo.o : AdcDemo.cpp
	$(GXX_COMPILER) $(CPPFLAGS) AdcDemo.cpp

AdcDemo : AdcDemo.o libpsn32.a AdcDemo.h
	$(GXX_COMPILER) -o AdcDemo AdcDemo.o -L. -lpsn32 -lpthread $(LDFLAGS)

.PHONY : clean
clean :
	rm -f *.o
	rm -f libpsn32.a
	rm -f AdcDemo

# end
