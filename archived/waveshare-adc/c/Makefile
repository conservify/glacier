# USELIB_RPI = USE_BCM2835_LIB
# USELIB_RPI = USE_WIRINGPI_LIB
USELIB_RPI = USE_DEV_LIB

DIR_LIB   = ./lib
DIR_MAIN  = ./examples
DIR_BUILD = ./build

OBJ_C = $(wildcard ${DIR_LIB}/*.c ${DIR_MAIN}/*.c)
OBJ_O = $(patsubst %.c,${DIR_BUILD}/%.o,$(notdir ${OBJ_C}))

ifeq ($(USELIB_RPI), USE_BCM2835_LIB)
    LIB_RPI = -lbcm2835 -lm 
else ifeq ($(USELIB_RPI), USE_WIRINGPI_LIB)
    LIB_RPI = -lwiringPi -lm 
else ifeq ($(USELIB_RPI), USE_DEV_LIB)
    LIB_RPI = -lm  -lpthread
endif

DEBUG = -D DEBUG
DEBUG_RPI = -D $(USELIB_RPI) -D RPI

.PHONY : RPI clean

RPI: RPI_DEV RPI_epd 

TARGET = main
CC = gcc
MSG = -g -O0 -Wall
CFLAGS += $(MSG)

RPI_epd: ${OBJ_O}
	$(CC) $(CFLAGS) -D RPI $(OBJ_O) -o $(TARGET) $(LIB_RPI) $(DEBUG)

${DIR_BUILD}/%.o: $(DIR_MAIN)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I $(DIR_LIB) $(DEBUG)
    
${DIR_BUILD}/%.o: $(DIR_LIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I $(DIR_LIB) $(DEBUG)

RPI_DEV: $(DIR_BUILD)
	$(CC) $(CFLAGS) $(DEBUG_RPI) -c $(DIR_LIB)/dev_hardware_spi.c -o $(DIR_BUILD)/dev_hardware_spi.o $(LIB_RPI) $(DEBUG)
	$(CC) $(CFLAGS) $(DEBUG_RPI) -c $(DIR_LIB)/sysfs_gpio.c -o $(DIR_BUILD)/sysfs_gpio.o $(LIB_RPI) $(DEBUG)
	$(CC) $(CFLAGS) $(DEBUG_RPI) -c $(DIR_LIB)/devices_config.c -o $(DIR_BUILD)/devices_config.o $(LIB_RPI) $(DEBUG)
	$(CC) $(CFLAGS) $(DEBUG_RPI) -c $(DIR_LIB)/tinywav.c -o $(DIR_BUILD)/tinywav.o $(LIB_RPI) $(DEBUG)

$(DIR_BUILD):
	mkdir -p $(DIR_BUILD)	

clean :
	rm -rf $(TARGET) $(DIR_BUILD)

