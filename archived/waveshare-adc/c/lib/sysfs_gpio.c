#include "sysfs_gpio.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int sysfs_gpio_export(int Pin) {
    char buffer[NUM_MAXBUF];
    int len;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        sysfs_gpio_debug("Export Failed: Pin%d\n", Pin);
        return -1;
    }

    len = snprintf(buffer, NUM_MAXBUF, "%d", Pin);
    write(fd, buffer, len);

    sysfs_gpio_debug("Export: Pin%d\r\n", Pin);

    close(fd);
    return 0;
}

int sysfs_gpio_unexport(int Pin) {
    char buffer[NUM_MAXBUF];
    int len;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        sysfs_gpio_debug("unexport Failed: Pin%d\n", Pin);
        return -1;
    }

    len = snprintf(buffer, NUM_MAXBUF, "%d", Pin);
    write(fd, buffer, len);

    sysfs_gpio_debug("Unexport: Pin%d\r\n", Pin);

    close(fd);
    return 0;
}

int sysfs_gpio_direction(int Pin, int Dir) {
    const char dir_str[] = "in\0out";
    char path[DIR_MAXSIZ];
    int fd;

    snprintf(path, DIR_MAXSIZ, "/sys/class/gpio/gpio%d/direction", Pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        sysfs_gpio_debug("Set Direction failed: Pin%d\n", Pin);
        return -1;
    }

    if (write(fd, &dir_str[Dir == SYSFS_GPIO_IN ? 0 : 3], Dir == SYSFS_GPIO_IN ? 2 : 3) < 0) {
        sysfs_gpio_debug("failed to set direction!\r\n");
        return -1;
    }

    if (Dir == SYSFS_GPIO_IN) {
        sysfs_gpio_debug("Pin%d:intput\r\n", Pin);
    } else {
        sysfs_gpio_debug("Pin%d:Output\r\n", Pin);
    }

    close(fd);
    return 0;
}

int sysfs_gpio_read(int Pin) {
    char path[DIR_MAXSIZ];
    char value_str[3];
    int fd;

    snprintf(path, DIR_MAXSIZ, "/sys/class/gpio/gpio%d/value", Pin);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        sysfs_gpio_debug("Read failed Pin%d\n", Pin);
        return -1;
    }

    if (read(fd, value_str, 3) < 0) {
        sysfs_gpio_debug("failed to read value!\n");
        return -1;
    }

    close(fd);
    return (atoi(value_str));
}

int sysfs_gpio_write(int Pin, int value) {
    const char s_values_str[] = "01";
    char path[DIR_MAXSIZ];
    int fd;

    snprintf(path, DIR_MAXSIZ, "/sys/class/gpio/gpio%d/value", Pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        sysfs_gpio_debug("Write failed : Pin%d,value = %d\n", Pin, value);
        return -1;
    }

    if (write(fd, &s_values_str[value == SYSFS_GPIO_LOW ? 0 : 1], 1) < 0) {
        sysfs_gpio_debug("failed to write value!\n");
        return -1;
    }

    close(fd);
    return 0;
}
