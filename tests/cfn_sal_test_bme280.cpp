/**
 * @file cfn_sal_test_bme280.cpp
 * @brief Unit tests for the BME280 composite sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/composite_sensor/bme280/cfn_sal_bme280.h"
#include "cfn_hal_i2c.h"

class Bme280Test : public ::testing::Test
{
  protected:
    cfn_sal_bme280_t     bme{};
    cfn_sal_phy_t        phy{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    void SetUp() override
    {
        memset(&bme, 0, sizeof(bme));
        memset(&phy, 0, sizeof(phy));
        memset(&i2c_dev, 0, sizeof(i2c_dev));

        /* Initialize I2C Mock */
        memset(&mock_i2c, 0, sizeof(mock_i2c));
        memset(&mock_i2c_api, 0, sizeof(mock_i2c_api));
        memset(&mock_i2c_cfg, 0, sizeof(mock_i2c_cfg));

        mock_i2c_api.base.init = [](cfn_hal_driver_t *d) -> cfn_hal_error_code_t
        {
            (void) d;
            return CFN_HAL_ERROR_OK;
        };
        mock_i2c_api.base.deinit = [](cfn_hal_driver_t *d) -> cfn_hal_error_code_t
        {
            (void) d;
            return CFN_HAL_ERROR_OK;
        };
        mock_i2c_api.mem_read =
            [](cfn_hal_i2c_t *d, const cfn_hal_i2c_mem_transaction_t *xfr, uint32_t t) -> cfn_hal_error_code_t
        {
            (void) d;
            (void) t;
            if (xfr->mem_addr == 0xD0)
            {
                xfr->data[0] = 0x60;
            } /* WHO_AM_I */
            return CFN_HAL_ERROR_OK;
        };
        mock_i2c_api.mem_write =
            [](cfn_hal_i2c_t *d, const cfn_hal_i2c_mem_transaction_t *xfr, uint32_t t) -> cfn_hal_error_code_t
        {
            (void) d;
            (void) xfr;
            (void) t;
            return CFN_HAL_ERROR_OK;
        };

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);

        i2c_dev.i2c     = &mock_i2c;
        i2c_dev.address = 0x76;

        phy.instance    = &i2c_dev;
        phy.type        = CFN_HAL_PERIPHERAL_TYPE_I2C;

        /* Construct the composite sensor */
        cfn_sal_bme280_construct(&bme, &phy, NULL, NULL);
    }
};

TEST_F(Bme280Test, SharedInitialization)
{
    /* Initialize temperature sensor */
    EXPECT_EQ(cfn_sal_temp_sensor_init(&bme.temp), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme.shared.init_ref_count, 1);
    EXPECT_TRUE(bme.shared.hw_initialized);

    /* Initialize humidity sensor */
    EXPECT_EQ(cfn_sal_hum_sensor_init(&bme.hum), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme.shared.init_ref_count, 2);

    /* Initialize pressure sensor */
    EXPECT_EQ(cfn_sal_pressure_sensor_init(&bme.press), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme.shared.init_ref_count, 3);

    /* Deinit one */
    EXPECT_EQ(cfn_sal_temp_sensor_deinit(&bme.temp), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme.shared.init_ref_count, 2);
    EXPECT_TRUE(bme.shared.hw_initialized);

    /* Deinit remaining */
    EXPECT_EQ(cfn_sal_hum_sensor_deinit(&bme.hum), CFN_HAL_ERROR_OK);
    EXPECT_EQ(cfn_sal_pressure_sensor_deinit(&bme.press), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme.shared.init_ref_count, 0);
    EXPECT_FALSE(bme.shared.hw_initialized);
}

TEST_F(Bme280Test, Getters)
{
    EXPECT_EQ(cfn_sal_bme280_get_temp(&bme), &bme.temp);
    EXPECT_EQ(cfn_sal_bme280_get_hum(&bme), &bme.hum);
    EXPECT_EQ(cfn_sal_bme280_get_press(&bme), &bme.press);
}
