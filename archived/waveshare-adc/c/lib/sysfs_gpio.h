#ifndef __SYSFS_GPIO_
#define __SYSFS_GPIO_

#include <stdio.h>

#define SYSFS_GPIO_IN  0
#define SYSFS_GPIO_OUT 1

#define SYSFS_GPIO_LOW  0
#define SYSFS_GPIO_HIGH 1

#define NUM_MAXBUF 4
#define DIR_MAXSIZ 60

#define SYSFS_GPIO_DEBUG 0
#if SYSFS_GPIO_DEBUG
#define sysfs_gpio_debug(__info, ...) printf("Debug: " __info, ##__VA_ARGS__)
#else
#define sysfs_gpio_debug(__info, ...)
#endif

#define GPIO4     4  // 7, 4
#define GPIO17    7  // 11, 17
#define GPIO18    18 // 12, 18
#define GPIO27    27 // 13, 27
#define GPIO22    22 // 15, 22
#define GPIO23    23 // 16, 23
#define GPIO24    24 // 18, 24
#define SPI0_MOSI 10 // 19, 10
#define SPI0_MISO 9  // 21, 9
#define GPIO25    28 // 22, 25
#define SPI0_SCK  11 // 23, 11
#define SPI0_CS0  8  // 24, 8
#define SPI0_CS1  7  // 26, 7
#define GPIO5     5  // 29, 5
#define GPIO6     6  // 31, 6
#define GPIO12    12 // 32, 12
#define GPIO13    13 // 33, 13
#define GPIO19    19 // 35, 19
#define GPIO16    16 // 36, 16
#define GPIO26    26 // 37, 26
#define GPIO20    20 // 38, 20
#define GPIO21    21 // 40, 21

int sysfs_gpio_export(int pin);
int sysfs_gpio_unexport(int pin);
int sysfs_gpio_direction(int pin, int dir);
int sysfs_gpio_read(int pin);
int sysfs_gpio_write(int pin, int value);

#endif