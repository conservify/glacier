#include <fcntl.h>

#include "devices_config.h"

int DEV_RST_PIN = 18;
int DEV_CS_PIN = 22;
int DEV_DRDY_PIN = 17;

void dev_digital_write(uint16_t pin, uint8_t value) {
    sysfs_gpio_write(pin, value);
}

uint8_t dev_digital_read(uint16_t pin) {
    return sysfs_gpio_read(pin);
}

uint8_t dev_spi_write_byte(uint8_t value) {
    return dev_hardware_spi_transfer_byte(value);
}

uint8_t dev_spi_read_byte() {
    return dev_spi_write_byte(0x00);
}

int32_t dev_spi_read_bytes(uint8_t *buf, uint32_t len) {
    return dev_hardware_spi_transfer(buf, len);
}

void dev_gpio_mode(uint16_t pin, uint16_t mode) {
    sysfs_gpio_export(pin);
    if (mode == 0 || mode == SYSFS_GPIO_IN) {
        sysfs_gpio_direction(pin, SYSFS_GPIO_IN);
    } else {
        sysfs_gpio_direction(pin, SYSFS_GPIO_OUT);
    }
}

void dev_delay_ms(uint32_t xms) {
    usleep(1000 * xms);
}

void dev_gpio_init() {
    DEV_RST_PIN = 18;
    DEV_CS_PIN = 22;
    DEV_DRDY_PIN = 17;

    dev_gpio_mode(DEV_RST_PIN, 1);
    dev_gpio_mode(DEV_CS_PIN, 1);
    dev_gpio_mode(DEV_DRDY_PIN, 0);
    dev_digital_write(DEV_CS_PIN, 1);
}

uint8_t dev_module_init() {
    dev_gpio_init();
    dev_hardware_spi_begin("/dev/spidev0.0");
    dev_hardware_spi_set_speed(1000000);
    dev_hardware_spi_mode(SPI_MODE_1);
    return 0;
}

void dev_module_exit() {
    dev_hardware_spi_end();
    dev_digital_write(DEV_RST_PIN, 0);
    dev_digital_write(DEV_CS_PIN, 0);
}
