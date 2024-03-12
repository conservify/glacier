#include "dev_hardware_spi.h"

#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

HARDWARE_SPI hardware_SPI;

static uint8_t bits = 8;

// #define SPI_CS_HIGH     0x04                //Chip select high
// #define SPI_LSB_FIRST   0x08                //LSB
// #define SPI_3WIRE       0x10                //3-wire mode SI and SO same line
// #define SPI_LOOP        0x20                //Loopback mode
// #define SPI_NO_CS       0x40                //A single device occupies one SPI bus, so there is no chip select
// #define SPI_READY       0x80                //Slave pull low to stop data transmission

struct spi_ioc_transfer tr;

void dev_hardware_spi_begin(char *SPI_device) {
    int ret = 0;
    if ((hardware_SPI.fd = open(SPI_device, O_RDWR)) < 0) {
        perror("Failed to open SPI device.\n");
        dev_hardware_spi_debug("Failed to open SPI device\r\n");
        exit(1);
    } else {
        dev_hardware_spi_debug("open : %s\r\n", SPI_device);
    }
    hardware_SPI.mode = 0;

    ret = ioctl(hardware_SPI.fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        dev_hardware_spi_debug("can't set bits per word\r\n");
    }

    ret = ioctl(hardware_SPI.fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1) {
        dev_hardware_spi_debug("can't get bits per word\r\n");
    }
    tr.bits_per_word = bits;

    dev_hardware_spi_mode(SPI_MODE_0);
    dev_hardware_spi_chip_select(SPI_CS_Mode_LOW);
    dev_hardware_spi_set_bit_order(SPI_BIT_ORDER_MSBFIRST);
    dev_hardware_spi_set_speed(20000000);
    dev_hardware_spi_set_data_interval(0);
}

void dev_hardware_spi_begin_set(char *SPI_device, SPIMode mode, uint32_t speed) {
    int32_t ret = 0;
    hardware_SPI.mode = 0;
    if ((hardware_SPI.fd = open(SPI_device, O_RDWR)) < 0) {
        perror("Failed to open SPI device.\n");
        exit(1);
    } else {
        dev_hardware_spi_debug("open : %s\r\n", SPI_device);
    }

    ret = ioctl(hardware_SPI.fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1) {
        dev_hardware_spi_debug("can't set bits per word\r\n");
    }

    ret = ioctl(hardware_SPI.fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1) {
        dev_hardware_spi_debug("can't get bits per word\r\n");
    }

    dev_hardware_spi_mode(mode);
    dev_hardware_spi_chip_select(SPI_CS_Mode_LOW);
    dev_hardware_spi_set_speed(speed);
    dev_hardware_spi_set_data_interval(0);
}

void dev_hardware_spi_end(void) {
    hardware_SPI.mode = 0;
    if (close(hardware_SPI.fd) != 0) {
        dev_hardware_spi_debug("Failed to close SPI device\r\n");
        perror("Failed to close SPI device.\n");
    }
}

int32_t dev_hardware_spi_set_speed(uint32_t speed) {
    uint32_t speed1 = hardware_SPI.speed;

    hardware_SPI.speed = speed;

    // Write speed
    if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        dev_hardware_spi_debug("can't set max speed hz\r\n");
        hardware_SPI.speed = speed1; // Setting failure rate unchanged
        return -1;
    }

    // Read the speed of just writing
    if (ioctl(hardware_SPI.fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) == -1) {
        dev_hardware_spi_debug("can't get max speed hz\r\n");
        hardware_SPI.speed = speed1; // Setting failure rate unchanged
        return -1;
    }
    hardware_SPI.speed = speed;
    tr.speed_hz = hardware_SPI.speed;
    return 1;
}

int32_t dev_hardware_spi_mode(SPIMode mode) {
    hardware_SPI.mode &= 0xfC; // Clear low 2 digits
    hardware_SPI.mode |= mode; // Setting mode

    // Write device
    if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode) == -1) {
        dev_hardware_spi_debug("can't set spi mode\r\n");
        return -1;
    }
    return 1;
}

int32_t dev_hardware_spi_csen(SPICSEN EN) {
    if (EN == ENABLE) {
        hardware_SPI.mode |= SPI_NO_CS;
    } else {
        hardware_SPI.mode &= ~SPI_NO_CS;
    }
    // Write device
    if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode) == -1) {
        dev_hardware_spi_debug("can't set spi CS EN\r\n");
        return -1;
    }
    return 1;
}

int32_t dev_hardware_spi_chip_select(SPIChipSelect CS_Mode) {
    if (CS_Mode == SPI_CS_Mode_HIGH) {
        hardware_SPI.mode |= SPI_CS_HIGH;
        hardware_SPI.mode &= ~SPI_NO_CS;
        dev_hardware_spi_debug("CS HIGH \r\n");
    } else if (CS_Mode == SPI_CS_Mode_LOW) {
        hardware_SPI.mode &= ~SPI_CS_HIGH;
        hardware_SPI.mode &= ~SPI_NO_CS;
    } else if (CS_Mode == SPI_CS_Mode_NONE) {
        hardware_SPI.mode |= SPI_NO_CS;
    }

    if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode) == -1) {
        dev_hardware_spi_debug("can't set spi mode\r\n");
        return -1;
    }
    return 1;
}

int32_t dev_hardware_spi_set_bit_order(SPIBitOrder Order) {
    if (Order == SPI_BIT_ORDER_LSBFIRST) {
        hardware_SPI.mode |= SPI_LSB_FIRST;
        dev_hardware_spi_debug("SPI_LSB_FIRST\r\n");
    } else if (Order == SPI_BIT_ORDER_MSBFIRST) {
        hardware_SPI.mode &= ~SPI_LSB_FIRST;
        dev_hardware_spi_debug("SPI_MSB_FIRST\r\n");
    }

    // dev_hardware_spi_debug("hardware_SPI.mode = 0x%02x\r\n", hardware_SPI.mode);
    int fd = ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode);
    dev_hardware_spi_debug("fd = %d\r\n", fd);
    if (fd == -1) {
        dev_hardware_spi_debug("can't set spi SPI_LSB_FIRST\r\n");
        return -1;
    }
    return 1;
}

int32_t dev_hardware_spi_set_bus_mode(BusMode mode) {
    if (mode == SPI_3WIRE_Mode) {
        hardware_SPI.mode |= SPI_3WIRE;
    } else if (mode == SPI_4WIRE_Mode) {
        hardware_SPI.mode &= ~SPI_3WIRE;
    }
    if (ioctl(hardware_SPI.fd, SPI_IOC_WR_MODE, &hardware_SPI.mode) == -1) {
        dev_hardware_spi_debug("can't set spi mode\r\n");
        return -1;
    }
    return 1;
}

void dev_hardware_spi_set_data_interval(uint16_t us) {
    hardware_SPI.delay = us;
    tr.delay_usecs = hardware_SPI.delay;
}

uint8_t dev_hardware_spi_transfer_byte(uint8_t buf) {
    uint8_t rbuf[1];
    tr.len = 1;
    tr.tx_buf = (unsigned long)&buf;
    tr.rx_buf = (unsigned long)rbuf;

    if (ioctl(hardware_SPI.fd, SPI_IOC_MESSAGE(1), &tr) < 1)
        dev_hardware_spi_debug("can't send spi message\r\n");
    return rbuf[0];
}

int32_t dev_hardware_spi_transfer(uint8_t *buf, uint32_t len) {
    tr.len = len;
    tr.tx_buf = (unsigned long)buf;
    tr.rx_buf = (unsigned long)buf;

    if (ioctl(hardware_SPI.fd, SPI_IOC_MESSAGE(1), &tr) < 1) {
        dev_hardware_spi_debug("can't send spi message\r\n");
        return -1;
    }

    return 1;
}
