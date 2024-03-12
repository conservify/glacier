#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"

#ifdef USE_BCM2835_LIB
#include <bcm2835.h>
#elif USE_WIRINGPI_LIB
#include <wiringPi.h>
#include <wiringPiSPI.h>
#elif USE_DEV_LIB
#include "sysfs_gpio.h"
#include "dev_hardware_spi.h"
#endif

extern int DEV_RST_PIN;
extern int DEV_CS_PIN;
extern int DEV_DRDY_PIN;

void dev_digital_write(uint16_t pin, uint8_t value);
uint8_t dev_digital_read(uint16_t pin);
uint8_t dev_spi_write_byte(uint8_t value);
uint8_t dev_spi_read_byte();
int32_t dev_spi_read_bytes(uint8_t *buf, uint32_t len);
uint8_t dev_module_init();
void dev_module_exit();
void dev_delay_ms(uint32_t xms);

#endif
