#ifndef __DEV_HARDWARE_SPI_
#define __DEV_HARDWARE_SPI_

#include <stdint.h>

#define DEV_HARDWARE_SPI_DEBUG 0
#if DEV_HARDWARE_SPI_DEBUG
#define dev_hardware_spi_debug(__info, ...) printf("Debug: " __info, ##__VA_ARGS__)
#else
#define dev_hardware_spi_debug(__info, ...)
#endif

#define SPI_CPHA   0x01
#define SPI_CPOL   0x02
#define SPI_MODE_0 (0 | 0)
#define SPI_MODE_1 (0 | SPI_CPHA)
#define SPI_MODE_2 (SPI_CPOL | 0)
#define SPI_MODE_3 (SPI_CPOL | SPI_CPHA)

typedef enum {
    SPI_MODE0 = SPI_MODE_0, /*!< CPOL = 0, CPHA = 0 */
    SPI_MODE1 = SPI_MODE_1, /*!< CPOL = 0, CPHA = 1 */
    SPI_MODE2 = SPI_MODE_2, /*!< CPOL = 1, CPHA = 0 */
    SPI_MODE3 = SPI_MODE_3  /*!< CPOL = 1, CPHA = 1 */
} SPIMode;

typedef enum { DISABLE = 0, ENABLE = 1 } SPICSEN;

typedef enum {
    SPI_CS_Mode_LOW = 0,  /*!< Chip Select 0 */
    SPI_CS_Mode_HIGH = 1, /*!< Chip Select 1 */
    SPI_CS_Mode_NONE = 3  /*!< No CS, control it yourself */
} SPIChipSelect;

typedef enum {
    SPI_BIT_ORDER_LSBFIRST = 0, /*!< LSB First */
    SPI_BIT_ORDER_MSBFIRST = 1  /*!< MSB First */
} SPIBitOrder;

typedef enum { SPI_3WIRE_Mode = 0, SPI_4WIRE_Mode = 1 } BusMode;

typedef struct SPIStruct {
    uint16_t SCLK_PIN;
    uint16_t MOSI_PIN;
    uint16_t MISO_PIN;
    uint16_t CS0_PIN;
    uint16_t CS1_PIN;
    uint32_t speed;
    uint16_t mode;
    uint16_t delay;
    int fd;
} HARDWARE_SPI;

void dev_hardware_spi_begin(char *SPI_device);
void dev_hardware_spi_begin_set(char *SPI_device, SPIMode mode, uint32_t speed);
void dev_hardware_spi_end(void);
int32_t dev_hardware_spi_set_speed(uint32_t speed);
uint8_t dev_hardware_spi_transfer_byte(uint8_t buf);
int32_t dev_hardware_spi_transfer(uint8_t *buf, uint32_t len);
void dev_hardware_spi_set_data_interval(uint16_t us);
int32_t dev_hardware_spi_set_bus_mode(BusMode mode);
int32_t dev_hardware_spi_set_bit_order(SPIBitOrder Order);
int32_t dev_hardware_spi_chip_select(SPIChipSelect CS_Mode);
int32_t dev_hardware_spi_csen(SPICSEN EN);
int32_t dev_hardware_spi_mode(SPIMode mode);

#endif