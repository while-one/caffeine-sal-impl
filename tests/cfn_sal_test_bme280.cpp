/**
 * @file cfn_sal_test_bme280.cpp
 * @brief Unit tests for the BME280 combined sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/combined_sensor/bme280/cfn_sal_bme280.h"
#include "cfn_hal_i2c.h"

class Bme280Test : public ::testing::Test
{
  protected:
    cfn_sal_bme280_ctx_t bme_ctx{};
    cfn_sal_phy_t        phy{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    cfn_sal_temp_sensor_t     temp_driver{};
    cfn_sal_hum_sensor_t      hum_driver{};
    cfn_sal_pressure_sensor_t press_driver{};

    void SetUp() override
    {
        memset(&bme_ctx, 0, sizeof(bme_ctx));
        memset(&phy, 0, sizeof(phy));

        /* Initialize I2C Mock */
        memset(&mock_i2c, 0, sizeof(mock_i2c));
        memset(&mock_i2c_api, 0, sizeof(mock_i2c_api));
        memset(&mock_i2c_cfg, 0, sizeof(mock_i2c_cfg));

        /* A minimal init implementation for the mock */
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

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);

        phy.handle   = &mock_i2c;
        phy.user_arg = &bme_ctx;

        /* Construct the three logical drivers */
        cfn_sal_bme280_temp_construct(&temp_driver, nullptr, &phy, nullptr, nullptr);
        cfn_sal_bme280_hum_construct(&hum_driver, nullptr, &phy, nullptr, nullptr);
        cfn_sal_bme280_pressure_construct(&press_driver, nullptr, &phy, nullptr, nullptr);
    }
};

TEST_F(Bme280Test, SharedInitialization)
{
    /* Initialize temperature sensor */
    EXPECT_EQ(cfn_sal_temp_sensor_init(&temp_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme_ctx.base.ref_count, 1);
    EXPECT_TRUE(bme_ctx.hw_initialized);

    /* Initialize humidity sensor - should NOT call I2C init again but increment ref_count */
    EXPECT_EQ(cfn_sal_hum_sensor_init(&hum_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme_ctx.base.ref_count, 2);

    /* Initialize pressure sensor */
    EXPECT_EQ(cfn_sal_pressure_sensor_init(&press_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme_ctx.base.ref_count, 3);

    /* Deinit one */
    EXPECT_EQ(cfn_sal_temp_sensor_deinit(&temp_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme_ctx.base.ref_count, 2);
    EXPECT_TRUE(bme_ctx.hw_initialized);

    /* Deinit remaining */
    EXPECT_EQ(cfn_sal_hum_sensor_deinit(&hum_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(cfn_sal_pressure_sensor_deinit(&press_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(bme_ctx.base.ref_count, 0);
    EXPECT_FALSE(bme_ctx.hw_initialized);
}

TEST_F(Bme280Test, DataReading)
{
    cfn_sal_temp_sensor_init(&temp_driver);

    float val = 0.0f;
    EXPECT_EQ(cfn_sal_temp_sensor_read_celsius(&temp_driver, &val), CFN_HAL_ERROR_OK);
    EXPECT_EQ(val, 25.0f);

    EXPECT_EQ(cfn_sal_hum_sensor_init(&hum_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(cfn_sal_hum_sensor_read_rh(&hum_driver, &val), CFN_HAL_ERROR_OK);
    EXPECT_EQ(val, 50.0f);

    EXPECT_EQ(cfn_sal_pressure_sensor_init(&press_driver), CFN_HAL_ERROR_OK);
    EXPECT_EQ(cfn_sal_pressure_sensor_read_hpa(&press_driver, &val), CFN_HAL_ERROR_OK);
    EXPECT_EQ(val, 1013.25f);
}
