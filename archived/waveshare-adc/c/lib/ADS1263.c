#include "ADS1263.h"

uint8_t ScanMode = 0;

static void ads1263_reset(void) {
    dev_digital_write(DEV_RST_PIN, 1);
    dev_delay_ms(300);
    dev_digital_write(DEV_RST_PIN, 0);
    dev_delay_ms(300);
    dev_digital_write(DEV_RST_PIN, 1);
    dev_delay_ms(300);
}

static void ads1263_write_cmd(uint8_t Cmd) {
    dev_digital_write(DEV_CS_PIN, 0);
    dev_spi_write_byte(Cmd);
    dev_digital_write(DEV_CS_PIN, 1);
}

static void ads1263_write_reg(uint8_t Reg, uint8_t data) {
    dev_digital_write(DEV_CS_PIN, 0);
    dev_spi_write_byte(CMD_WREG | Reg);
    dev_spi_write_byte(0x00);
    dev_spi_write_byte(data);
    dev_digital_write(DEV_CS_PIN, 1);
}

static uint8_t ads1263_read_data(uint8_t reg) {
    uint8_t temp = 0;
    dev_digital_write(DEV_CS_PIN, 0);
    dev_spi_write_byte(CMD_RREG | reg);
    dev_spi_write_byte(0x00);
    temp = dev_spi_read_byte();
    dev_digital_write(DEV_CS_PIN, 1);
    return temp;
}

static uint8_t ads1263_checksum(uint32_t val, uint8_t byt) {
    uint8_t sum = 0;
    uint8_t mask = -1; // 8 bits mask, 0xff
    while (val) {
        sum += val & mask; // only add the lower values
        val >>= 8;         // shift down
    }
    sum += 0x9b;
    return sum ^ byt; // if equal, this will be 0
}

static void ads1263_wait_drdy(void) {
    uint32_t i = 0;
    while (1) {
        if (dev_digital_read(DEV_DRDY_PIN) == 0)
            break;
        if (i >= 4000000) {
            printf("Time Out ...\r\n");
            break;
        }
    }
}

uint8_t ads1263_read_chip_id(void) {
    uint8_t id = ads1263_read_data(REG_ID);
    return id >> 5;
}

void ads1263_set_mode(uint8_t Mode) {
    if (Mode == 0) {
        ScanMode = 0;
    } else {
        ScanMode = 1;
    }
}

void ads1263_config_adc1(ADS1263_GAIN gain, ADS1263_DRATE drate, ADS1263_DELAY delay) {
    uint8_t MODE2 = 0x80; // 0x80:PGA bypassed, 0x00:PGA enabled
    MODE2 |= (gain << 4) | drate;
    ads1263_write_reg(REG_MODE2, MODE2);
    dev_delay_ms(1);
    if (ads1263_read_data(REG_MODE2) != MODE2) {
        fprintf(stderr, "REG_MODE2 failed\n");
    }

    uint8_t REFMUX = 0x00; // 0x00:+-2.5V as REF, 0x24:VDD,VSS as REF
    ads1263_write_reg(REG_REFMUX, REFMUX);
    dev_delay_ms(1);
    if (ads1263_read_data(REG_REFMUX) != REFMUX) {
        fprintf(stderr, "REG_REFMUX failed\n");
    }

    uint8_t MODE0 = delay;
    ads1263_write_reg(REG_MODE0, MODE0);
    dev_delay_ms(1);
    if (ads1263_read_data(REG_MODE0) != MODE0) {
        fprintf(stderr, "REG_MODE0 failed\n");
    }

    uint8_t MODE1 = 0x64; // Digital Filter; 0x84:FIR, 0x64:Sinc4, 0x44:Sinc3, 0x24:Sinc2, 0x04:Sinc1
    ads1263_write_reg(REG_MODE1, MODE1);
    dev_delay_ms(1);
    if (ads1263_read_data(REG_MODE1) != MODE1) {
        fprintf(stderr, "REG_MODE1 failed\n");
    }
}

uint8_t ads1263_init_adc1(ADS1263_DRATE rate) {
    ads1263_reset();
    if (ads1263_read_chip_id() != 1) {
        fprintf(stderr, "ads1263_read_chip_id failed\n");
        return 1;
    }
    ads1263_write_cmd(CMD_STOP1);
    ads1263_config_adc1(ADS1263_GAIN_8, rate, ADS1263_DELAY_69us);
    ads1263_write_cmd(CMD_START1);
    return 0;
}

static void ads1263_set_channel(uint8_t channel) {
    if (channel > 10) {
        return;
    }
    uint8_t INPMUX = (channel << 4) | 0x0a; // 0x0a:VCOM as Negative Input
    ads1263_write_reg(REG_INPMUX, INPMUX);
    if (ads1263_read_data(REG_INPMUX) != INPMUX) {
        fprintf(stderr, "ADS1263_ADC1_SetChannel failed\n");
    }
}

void ads1263_set_diff_channel(uint8_t channel) {
    uint8_t INPMUX;

    if (channel == 0) {
        INPMUX = (0 << 4) | 1; // DiffChannel AIN0-AIN1
    } else if (channel == 1) {
        INPMUX = (2 << 4) | 3; // DiffChannel AIN2-AIN3
    } else if (channel == 2) {
        INPMUX = (4 << 4) | 5; // DiffChannel AIN4-AIN5
    } else if (channel == 3) {
        INPMUX = (6 << 4) | 7; // DiffChannel AIN6-AIN7
    } else if (channel == 4) {
        INPMUX = (8 << 4) | 9; // DiffChannel AIN8-AIN9
    }
    ads1263_write_reg(REG_INPMUX, INPMUX);
    /*
    if (ads1263_read_data(REG_INPMUX) != INPMUX) {
        fprintf(stderr, "ADS1263_SetDiffChannel failed\n");
    }
    */
}

static uint32_t ads1263_read_adc1_data() {
    uint32_t read = 0;
    uint8_t buf[5] = { 0, 0, 0, 0 };
    uint8_t status, crc;

    dev_digital_write(DEV_CS_PIN, 0);
    do {
        dev_spi_write_byte(CMD_RDATA1);
        status = dev_spi_read_byte();
    } while ((status & 0x40) == 0);

    dev_spi_read_bytes(buf, sizeof(buf));
    // buf[0] = dev_spi_read_byte();
    // buf[1] = dev_spi_read_byte();
    // buf[2] = dev_spi_read_byte();
    // buf[3] = dev_spi_read_byte();
    // crc = dev_spi_read_byte();
    crc = buf[4];
    dev_digital_write(DEV_CS_PIN, 1);

    /*
    uint32_t temp = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    uint32_t other = (uint32_t)temp;
    if (ads1263_checksum(other, crc) != 0) {
        fprintf(stderr, "ADC1 checksum failed\n");
    }
    */

    read |= ((uint32_t)buf[0] << 24);
    read |= ((uint32_t)buf[1] << 16);
    read |= ((uint32_t)buf[2] << 8);
    read |= (uint32_t)buf[3];

    /*
    if (ads1263_checksum(read, crc) != 0) {
        fprintf(stderr, "ADC1 checksum failed\n");
    }
    */

    return read;
}

uint32_t ads1263_get_channel_value(uint8_t channel) {
    uint32_t value = 0;
    if (ScanMode == 0) { // 0  Single-ended input  10 channel1 Differential input  5 channe
        if (channel > 10) {
            return 0;
        }
        ads1263_set_channel(channel);
        // dev_delay_ms(2);
        // ads1263_writecmd(CMD_START1);
        // dev_delay_ms(2);
        ads1263_wait_drdy();
        value = ads1263_read_adc1_data();
    } else {
        if (channel > 4) {
            return 0;
        }
        ads1263_set_diff_channel(channel);
        // dev_delay_ms(2);
        // ads1263_writecmd(CMD_START1);
        // dev_delay_ms(2);
        ads1263_wait_drdy();
        value = ads1263_read_adc1_data();
    }
    return value;
}

void ads1263_get_all(uint32_t *values, int32_t channels) {
    uint8_t i;
    for (i = 0; i < channels; i++) {
        values[i] = ads1263_get_channel_value(i);
        // ads1263_writecmd(CMD_STOP1);
        // dev_delay_ms(20);
    }
}

void ads1263_get_channels_diff(uint32_t *values, int32_t channels) {
    for (int32_t c = 0; c < channels; c++) {
        ads1263_set_diff_channel(c);
        // ads1263_wait_drdy();
        values[c] = ads1263_read_adc1_data();
    }
}

void ads1263_get_multiple(uint32_t *values, int32_t channels, int32_t samples) {
    for (int32_t s = 0; s < samples; s++) {
        for (int32_t c = 0; c < channels; c++) {
            ads1263_set_diff_channel(c);
            ads1263_wait_drdy();
            values[s * channels + c] = ads1263_get_channel_value(c);
        }
    }
}

void ads1263_config_adc2(ADS1263_ADC2_GAIN gain, ADS1263_ADC2_DRATE drate, ADS1263_DELAY delay) {
    uint8_t ADC2CFG = 0x20; // REF, 0x20:VAVDD and VAVSS, 0x00:+-2.5V
    ADC2CFG |= (drate << 6) | gain;
    ads1263_write_reg(REG_ADC2CFG, ADC2CFG);
    dev_delay_ms(1);
    if (ads1263_read_data(REG_ADC2CFG) == ADC2CFG)
        printf("REG_ADC2CFG success \r\n");
    else
        printf("REG_ADC2CFG unsuccess \r\n");

    uint8_t MODE0 = delay;
    ads1263_write_reg(REG_MODE0, MODE0);
    dev_delay_ms(1);
    if (ads1263_read_data(REG_MODE0) == MODE0)
        printf("REG_MODE0 success \r\n");
    else
        printf("REG_MODE0 unsuccess \r\n");
}

uint8_t ads1263_init_adc2(ADS1263_ADC2_DRATE rate) {
    ads1263_reset();
    if (ads1263_read_chip_id() == 1) {
        printf("ID Read success \r\n");
    } else {
        printf("ID Read failed \r\n");
        return 1;
    }
    ads1263_write_cmd(CMD_STOP2);
    ads1263_config_adc2(ADS1263_ADC2_GAIN_4, rate, ADS1263_DELAY_35us);
    return 0;
}

void ads1263_set_diff_channel_adc2(uint8_t channel) {
    uint8_t INPMUX;
    if (channel == 0) {
        INPMUX = (0 << 4) | 1; // DiffChannel   AIN0-AIN1
    } else if (channel == 1) {
        INPMUX = (2 << 4) | 3; // DiffChannel   AIN2-AIN3
    } else if (channel == 2) {
        INPMUX = (4 << 4) | 5; // DiffChannel   AIN4-AIN5
    } else if (channel == 3) {
        INPMUX = (6 << 4) | 7; // DiffChannel   AIN6-AIN7
    } else if (channel == 4) {
        INPMUX = (8 << 4) | 9; // DiffChannel   AIN8-AIN9
    }
    ads1263_write_reg(REG_ADC2MUX, INPMUX);
    if (ads1263_read_data(REG_ADC2MUX) == INPMUX) {
        // printf("ADS1263_SetDiffChannel_ADC2 success \r\n");
    } else {
        printf("ADS1263_SetDiffChannel_ADC2 unsuccess \r\n");
    }
}

static void ads1263_set_channel_adc2(uint8_t channel) {
    if (channel > 10) {
        return;
    }
    uint8_t INPMUX = (channel << 4) | 0x0a; // 0x0a:VCOM as Negative Input
    ads1263_write_reg(REG_ADC2MUX, INPMUX);
    if (ads1263_read_data(REG_ADC2MUX) == INPMUX) {
        // printf("ADS1263_ADC2_SetChannel success \r\n");
    } else {
        printf("ADS1263_ADC2_SetChannel unsuccess \r\n");
    }
}

static uint32_t ads1263_read_adc2_data(void) {
    uint32_t read = 0;
    uint8_t buf[4] = { 0, 0, 0, 0 };
    uint8_t Status, CRC;

    dev_digital_write(DEV_CS_PIN, 0);
    do {
        dev_spi_write_byte(CMD_RDATA2);
        // dev_delay_ms(10);
        Status = dev_spi_read_byte();
    } while ((Status & 0x80) == 0);

    buf[0] = dev_spi_read_byte();
    buf[1] = dev_spi_read_byte();
    buf[2] = dev_spi_read_byte();
    buf[3] = dev_spi_read_byte();
    CRC = dev_spi_read_byte();
    dev_digital_write(DEV_CS_PIN, 1);
    read |= ((uint32_t)buf[0] << 16);
    read |= ((uint32_t)buf[1] << 8);
    read |= (uint32_t)buf[2];
    // printf("%x %x %x %x %x\r\n", Status, buf[0], buf[1], buf[2], CRC);
    if (ads1263_checksum(read, CRC) != 0)
        printf("ADC2 Data read error! \r\n");
    return read;
}

uint32_t ads1263_get_channel_value_adc2(uint8_t channel) {
    uint32_t Value = 0;
    if (ScanMode == 0) { // 0  Single-ended input  10 channel1 Differential input  5 channe
        if (channel > 10) {
            return 0;
        }
        ads1263_set_channel_adc2(channel);
        // dev_delay_ms(2);
        ads1263_write_cmd(CMD_START2);
        // dev_delay_ms(2);
        Value = ads1263_read_adc2_data();
    } else {
        if (channel > 4) {
            return 0;
        }
        ads1263_set_diff_channel_adc2(channel);
        // dev_delay_ms(2);
        ads1263_write_cmd(CMD_START2);
        // dev_delay_ms(2);
        Value = ads1263_read_adc2_data();
    }
    // printf("Get IN%d value success \r\n", channel);
    return Value;
}

void ads1263_get_all_adc2(uint32_t *ADC_Value) {
    uint8_t i;
    for (i = 0; i < 10; i++) {
        ADC_Value[i] = ads1263_get_channel_value_adc2(i);
        ads1263_write_cmd(CMD_STOP2);
        // dev_delay_ms(20);
    }
    // printf("----------Read ADC2 value success----------\r\n");
}

uint32_t ads1263_rtd(ADS1263_DELAY delay, ADS1263_GAIN gain, ADS1263_DRATE drate) {
    uint32_t Value;

    // MODE0 (CHOP OFF)
    uint8_t MODE0 = delay;
    ads1263_write_reg(REG_MODE0, MODE0);
    dev_delay_ms(1);

    //(IDACMUX) IDAC2 AINCOM,IDAC1 AIN3
    uint8_t IDACMUX = (0x0a << 4) | 0x03;
    ads1263_write_reg(REG_IDACMUX, IDACMUX);
    dev_delay_ms(1);

    //((IDACMAG)) IDAC2 = IDAC1 = 250uA
    uint8_t IDACMAG = (0x03 << 4) | 0x03;
    ads1263_write_reg(REG_IDACMAG, IDACMAG);
    dev_delay_ms(1);

    uint8_t MODE2 = (gain << 4) | drate;
    ads1263_write_reg(REG_MODE2, MODE2);
    dev_delay_ms(1);

    // INPMUX (AINP = AIN7, AINN = AIN6)
    uint8_t INPMUX = (0x07 << 4) | 0x06;
    ads1263_write_reg(REG_INPMUX, INPMUX);
    dev_delay_ms(1);

    // REFMUX AIN4 AIN5
    uint8_t REFMUX = (0x03 << 3) | 0x03;
    ads1263_write_reg(REG_REFMUX, REFMUX);
    dev_delay_ms(1);

    // Read one conversion
    ads1263_write_cmd(CMD_START1);
    dev_delay_ms(10);
    ads1263_wait_drdy();
    Value = ads1263_read_adc1_data();
    ads1263_write_cmd(CMD_STOP1);

    return Value;
}

void ads1263_dac(ADS1263_DAC_VOLT volt, uint8_t isPositive, uint8_t isOpen) {
    uint8_t Reg, Value;

    if (isPositive)
        Reg = REG_TDACP; // IN6
    else
        Reg = REG_TDACN; // IN7

    if (isOpen)
        Value = volt | 0x80;
    else
        Value = 0x00;

    ads1263_write_reg(Reg, Value);
}
