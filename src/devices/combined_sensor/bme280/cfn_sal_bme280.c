/**
 * @file cfn_sal_bme280.c
 * @brief BME280 combined sensor implementation.
 */

#include "cfn_sal_bme280.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_spi.h"
#include <stddef.h>

/* -------------------------------------------------------------------------- */
/* Constants & Definitions                                                    */
/* -------------------------------------------------------------------------- */

#define BME280_REG_CALIB_00  0x88
#define BME280_REG_ID        0xD0
#define BME280_REG_RESET     0xE0
#define BME280_REG_CALIB_26  0xE1
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB  0xFA
#define BME280_REG_HUM_MSB   0xFD

#define BME280_ID_VAL    0x60
#define BME280_RESET_VAL 0xB6

#define BME280_MODE_SLEEP  0x00
#define BME280_MODE_FORCED 0x01
#define BME280_MODE_NORMAL 0x03

/* -------------------------------------------------------------------------- */
/* Internal Helpers                                                           */
/* -------------------------------------------------------------------------- */

static cfn_sal_bme280_t *get_bme280_from_base(cfn_hal_driver_t *base)
{
    if (base->type == CFN_SAL_TYPE_TEMP_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_bme280_t, temp.base);
    }
    else if (base->type == CFN_SAL_TYPE_HUM_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_bme280_t, hum.base);
    }
    else if (base->type == CFN_SAL_TYPE_PRESSURE_SENSOR)
    {
        return CFN_HAL_CONTAINER_OF(base, cfn_sal_bme280_t, press.base);
    }
    return NULL;
}

static cfn_hal_error_code_t bme280_read_regs(const cfn_sal_phy_t *phy, uint8_t reg, uint8_t *data, size_t size)
{
    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
    {
        cfn_hal_i2c_device_t         *dev = (cfn_hal_i2c_device_t *) phy->instance;
        cfn_hal_i2c_mem_transaction_t xfr = {
            .dev_addr = dev->address, .mem_addr = reg, .mem_addr_size = 1, .data = data, .size = size
        };
        return cfn_hal_i2c_mem_read(dev->i2c, &xfr, 100);
    }
    else if (phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
    {
        cfn_hal_spi_device_t     *dev  = (cfn_hal_spi_device_t *) phy->instance;
        uint8_t                   addr = reg | 0x80;
        cfn_hal_spi_transaction_t xfr  = { .tx_payload      = &addr,
                                           .nbr_of_tx_bytes = 1,
                                           .rx_payload      = data,
                                           .nbr_of_rx_bytes = size,
                                           .cs_pin          = dev->cs_pin };
        return cfn_hal_spi_xfr_polling(dev->spi, &xfr, 100);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

static cfn_hal_error_code_t bme280_write_reg(const cfn_sal_phy_t *phy, uint8_t reg, uint8_t val)
{
    if (phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
    {
        cfn_hal_i2c_device_t         *dev = (cfn_hal_i2c_device_t *) phy->instance;
        cfn_hal_i2c_mem_transaction_t xfr = {
            .dev_addr = dev->address, .mem_addr = reg, .mem_addr_size = 1, .data = &val, .size = 1
        };
        return cfn_hal_i2c_mem_write(dev->i2c, &xfr, 100);
    }
    else if (phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
    {
        cfn_hal_spi_device_t     *dev       = (cfn_hal_spi_device_t *) phy->instance;
        uint8_t                   buffer[2] = { reg & 0x7F, val };
        cfn_hal_spi_transaction_t xfr       = {
                  .tx_payload = buffer, .nbr_of_tx_bytes = 2, .rx_payload = NULL, .nbr_of_rx_bytes = 0, .cs_pin = dev->cs_pin
        };
        return cfn_hal_spi_xfr_polling(dev->spi, &xfr, 100);
    }
    return CFN_HAL_ERROR_NOT_SUPPORTED;
}

/* -------------------------------------------------------------------------- */
/* Compensation Math (Bosch Sensortec Reference)                              */
/* -------------------------------------------------------------------------- */

static int32_t bme280_compensate_T(cfn_sal_bme280_t *bme, int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t) bme->calib.dig_T1 << 1))) * ((int32_t) bme->calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t) bme->calib.dig_T1)) * ((adc_T >> 4) - ((int32_t) bme->calib.dig_T1))) >> 12) *
            ((int32_t) bme->calib.dig_T3)) >>
           14;
    bme->calib.t_fine = var1 + var2;
    T                 = (bme->calib.t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t bme280_compensate_P(cfn_sal_bme280_t *bme, int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t) bme->calib.t_fine) - 128000;
    var2 = var1 * var1 * (int64_t) bme->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t) bme->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t) bme->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t) bme->calib.dig_P3) >> 8) + ((var1 * (int64_t) bme->calib.dig_P2) << 12);
    var1 = (((((int64_t) 1) << 47) + var1)) * ((int64_t) bme->calib.dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t) bme->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t) bme->calib.dig_P8) * p) >> 19;
    p    = ((p + var1 + var2) >> 8) + (((int64_t) bme->calib.dig_P7) << 4);
    return (uint32_t) p;
}

static uint32_t bme280_compensate_H(cfn_sal_bme280_t *bme, int32_t adc_H)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (bme->calib.t_fine - ((int32_t) 76800));
    v_x1_u32r =
        (((((adc_H << 14) - (((int32_t) bme->calib.dig_H4) << 20) - (((int32_t) bme->calib.dig_H5) * v_x1_u32r)) +
           ((int32_t) 16384)) >>
          15) *
         (((((((v_x1_u32r * ((int32_t) bme->calib.dig_H6)) >> 10) *
              (((v_x1_u32r * ((int32_t) bme->calib.dig_H3)) >> 11) + ((int32_t) 32768))) >>
             10) +
            ((int32_t) 2097152)) *
               ((int32_t) bme->calib.dig_H2) +
           8192) >>
          14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t) bme->calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t) (v_x1_u32r >> 12);
}

/* -------------------------------------------------------------------------- */
/* Shared Lifecycle Implementation                                            */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_shared_init(cfn_hal_driver_t *base)
{
    cfn_sal_bme280_t *bme = get_bme280_from_base(base);
    if (!bme || !bme->combined_state.phy || !bme->combined_state.phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (bme->combined_state.init_ref_count == 0)
    {
        /* Initialize Physical Bus */
        if (bme->combined_state.phy->type == CFN_HAL_PERIPHERAL_TYPE_I2C)
        {
            cfn_hal_i2c_device_t *dev = (cfn_hal_i2c_device_t *) bme->combined_state.phy->instance;
            cfn_hal_i2c_init(dev->i2c);
        }
        else if (bme->combined_state.phy->type == CFN_HAL_PERIPHERAL_TYPE_SPI)
        {
            cfn_hal_spi_device_t *dev = (cfn_hal_spi_device_t *) bme->combined_state.phy->instance;
            cfn_hal_spi_init(dev->spi);
        }

        /* WHO_AM_I */
        uint8_t id = 0;
        if (bme280_read_regs(bme->combined_state.phy, BME280_REG_ID, &id, 1) != CFN_HAL_ERROR_OK || id != BME280_ID_VAL)
        {
            return CFN_HAL_ERROR_FAIL;
        }

        /* Reset */
        (void) bme280_write_reg(bme->combined_state.phy, BME280_REG_RESET, BME280_RESET_VAL);
        /* cfn_hal_delay(10); */

        /* Read Calibration Data */
        uint8_t calib1[26];
        uint8_t calib2[7];
        if (bme280_read_regs(bme->combined_state.phy, BME280_REG_CALIB_00, calib1, 26) != CFN_HAL_ERROR_OK)
        {
            return CFN_HAL_ERROR_FAIL;
        }
        if (bme280_read_regs(bme->combined_state.phy, BME280_REG_CALIB_26, calib2, 7) != CFN_HAL_ERROR_OK)
        {
            return CFN_HAL_ERROR_FAIL;
        }

        bme->calib.dig_T1  = (calib1[1] << 8) | calib1[0];
        bme->calib.dig_T2  = (calib1[3] << 8) | calib1[2];
        bme->calib.dig_T3  = (calib1[5] << 8) | calib1[4];
        bme->calib.dig_P1  = (calib1[7] << 8) | calib1[0];
        bme->calib.dig_P2  = (calib1[9] << 8) | calib1[8];
        bme->calib.dig_P3  = (calib1[11] << 8) | calib1[10];
        bme->calib.dig_P4  = (calib1[13] << 8) | calib1[12];
        bme->calib.dig_P5  = (calib1[15] << 8) | calib1[14];
        bme->calib.dig_P6  = (calib1[17] << 8) | calib1[16];
        bme->calib.dig_P7  = (calib1[19] << 8) | calib1[18];
        bme->calib.dig_P8  = (calib1[21] << 8) | calib1[20];
        bme->calib.dig_P9  = (calib1[23] << 8) | calib1[22];
        bme->calib.dig_H1  = calib1[25];
        bme->calib.dig_H2  = (calib2[1] << 8) | calib2[0];
        bme->calib.dig_H3  = calib2[2];
        bme->calib.dig_H4  = (calib2[3] << 4) | (calib2[4] & 0x0F);
        bme->calib.dig_H5  = (calib2[5] << 4) | (calib2[4] >> 4);
        bme->calib.dig_H6  = (int8_t) calib2[6];

        /* Default Config: 1x oversampling, normal mode */
        bme->ctrl_hum_reg  = 0x01;
        bme->ctrl_meas_reg = 0x27; // 1x P, 1x T, normal mode
        bme->config_reg    = 0x00;

        (void) bme280_write_reg(bme->combined_state.phy, BME280_REG_CTRL_HUM, bme->ctrl_hum_reg);
        (void) bme280_write_reg(bme->combined_state.phy, BME280_REG_CTRL_MEAS, bme->ctrl_meas_reg);
        (void) bme280_write_reg(bme->combined_state.phy, BME280_REG_CONFIG, bme->config_reg);

        bme->combined_state.hw_initialized = true;
    }

    bme->combined_state.init_ref_count++;
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_shared_deinit(cfn_hal_driver_t *base)
{
    cfn_sal_bme280_t *bme = get_bme280_from_base(base);
    if (!bme)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    if (bme->combined_state.init_ref_count > 0)
    {
        bme->combined_state.init_ref_count--;
        if (bme->combined_state.init_ref_count == 0)
        {
            /* Power down */
            (void) bme280_write_reg(bme->combined_state.phy, BME280_REG_CTRL_MEAS, BME280_MODE_SLEEP);
            bme->combined_state.hw_initialized = false;
        }
    }
    return CFN_HAL_ERROR_OK;
}

static cfn_hal_error_code_t bme280_perform_read(cfn_sal_bme280_t *bme)
{
    uint8_t              buffer[8];
    cfn_hal_error_code_t err = bme280_read_regs(bme->combined_state.phy, BME280_REG_PRESS_MSB, buffer, 8);
    if (err != CFN_HAL_ERROR_OK)
    {
        return err;
    }

    int32_t adc_P     = (int32_t) (((uint32_t) buffer[0] << 12) | ((uint32_t) buffer[1] << 4) | (buffer[2] >> 4));
    int32_t adc_T     = (int32_t) (((uint32_t) buffer[3] << 12) | ((uint32_t) buffer[4] << 4) | (buffer[5] >> 4));
    int32_t adc_H     = (int32_t) (((uint32_t) buffer[6] << 8) | buffer[7]);

    int32_t  comp_T   = bme280_compensate_T(bme, adc_T);
    uint32_t comp_P   = bme280_compensate_P(bme, adc_P);
    uint32_t comp_H   = bme280_compensate_H(bme, adc_H);

    bme->cached_temp  = (float) comp_T / 100.0f;
    bme->cached_press = (float) comp_P / 256.0f / 100.0f; // hPa
    bme->cached_hum   = (float) comp_H / 1024.0f;

    return CFN_HAL_ERROR_OK;
}

/* -------------------------------------------------------------------------- */
/* Interface Implementations                                                  */
/* -------------------------------------------------------------------------- */

static cfn_hal_error_code_t bme280_temp_read_celsius(cfn_sal_temp_sensor_t *driver, float *temp_out)
{
    if (!driver || !temp_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_bme280_t    *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, temp);
    cfn_hal_error_code_t err = bme280_perform_read(bme);
    if (err == CFN_HAL_ERROR_OK)
    {
        *temp_out = bme->cached_temp;
    }
    return err;
}

static cfn_hal_error_code_t bme280_hum_read_rh(cfn_sal_hum_sensor_t *driver, float *hum_out)
{
    if (!driver || !hum_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_bme280_t    *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, hum);
    cfn_hal_error_code_t err = bme280_perform_read(bme);
    if (err == CFN_HAL_ERROR_OK)
    {
        *hum_out = bme->cached_hum;
    }
    return err;
}

static cfn_hal_error_code_t bme280_press_read_hpa(cfn_sal_pressure_sensor_t *driver, float *hpa_out)
{
    if (!driver || !hpa_out)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }
    cfn_sal_bme280_t    *bme = CFN_HAL_CONTAINER_OF(driver, cfn_sal_bme280_t, press);
    cfn_hal_error_code_t err = bme280_perform_read(bme);
    if (err == CFN_HAL_ERROR_OK)
    {
        *hpa_out = bme->cached_press;
    }
    return err;
}

static const cfn_sal_temp_sensor_api_t TEMP_API = {
    .base         = { .init = bme280_shared_init, .deinit = bme280_shared_deinit },
    .read_celsius = bme280_temp_read_celsius,
};

static const cfn_sal_hum_sensor_api_t HUM_API = {
    .base                   = { .init = bme280_shared_init, .deinit = bme280_shared_deinit },
    .read_relative_humidity = bme280_hum_read_rh,
};

static const cfn_sal_pressure_sensor_api_t PRESS_API = {
    .base     = { .init = bme280_shared_init, .deinit = bme280_shared_deinit },
    .read_hpa = bme280_press_read_hpa,
};

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

cfn_hal_error_code_t cfn_sal_bme280_construct(cfn_sal_bme280_t *sensor, const cfn_sal_phy_t *phy)
{
    if (!sensor || !phy || !phy->instance)
    {
        return CFN_HAL_ERROR_BAD_PARAM;
    }

    sensor->combined_state.phy            = phy;
    sensor->combined_state.init_ref_count = 0;
    sensor->combined_state.hw_initialized = false;

    cfn_sal_temp_sensor_populate(&sensor->temp, 0, &TEMP_API, phy, NULL, NULL, NULL);
    cfn_sal_hum_sensor_populate(&sensor->hum, 0, &HUM_API, phy, NULL, NULL, NULL);
    cfn_sal_pressure_sensor_populate(&sensor->press, 0, &PRESS_API, phy, NULL, NULL, NULL);

    return CFN_HAL_ERROR_OK;
}

cfn_hal_error_code_t cfn_sal_bme280_destruct(const cfn_sal_bme280_t *sensor)
{
    CFN_HAL_UNUSED(sensor);
    return CFN_HAL_ERROR_OK;
}
