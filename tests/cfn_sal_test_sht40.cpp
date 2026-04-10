/**
 * @file cfn_sal_test_sht40.cpp
 * @brief Unit tests for the SHT40 composite sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/composite_sensor/sht40/cfn_sal_sht40.h"
#include "cfn_hal_i2c.h"

class Sht40Test : public ::testing::Test
{
  protected:
    cfn_sal_sht40_t      sht{};
    cfn_sal_phy_t        phy{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    void SetUp() override
    {
        memset(&sht, 0, sizeof(sht));
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
        mock_i2c_api.xfr_polling =
            [](cfn_hal_i2c_t *d, const cfn_hal_i2c_transaction_t *xfr, uint32_t timeout) -> cfn_hal_error_code_t
        {
            (void) d;
            (void) timeout;
            if (xfr->rx_payload && xfr->nbr_of_rx_bytes >= 6)
            {
                /* Mock data: T=25C, RH=50% */
                xfr->rx_payload[0] = 0x66;
                xfr->rx_payload[1] = 0x66;
                xfr->rx_payload[2] = 0xCC; /* Dummy CRC */
                xfr->rx_payload[3] = 0x72;
                xfr->rx_payload[4] = 0xAF;
                xfr->rx_payload[5] = 0xCC; /* Dummy CRC */
            }
            return CFN_HAL_ERROR_OK;
        };

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);

        i2c_dev.i2c     = &mock_i2c;
        i2c_dev.address = CFN_SAL_SHT40_ADDR_DEFAULT;

        phy.instance    = &i2c_dev;
        phy.type        = CFN_HAL_PERIPHERAL_TYPE_I2C;

        cfn_sal_sht40_construct(&sht, &phy, NULL);
    }
};

TEST_F(Sht40Test, Lifecycle)
{
    EXPECT_EQ(cfn_sal_temp_sensor_init(&sht.temp), CFN_HAL_ERROR_OK);
    EXPECT_EQ(sht.shared.init_ref_count, 1);
    EXPECT_TRUE(sht.shared.hw_initialized);

    EXPECT_EQ(cfn_sal_hum_sensor_init(&sht.hum), CFN_HAL_ERROR_OK);
    EXPECT_EQ(sht.shared.init_ref_count, 2);

    EXPECT_EQ(cfn_sal_temp_sensor_deinit(&sht.temp), CFN_HAL_ERROR_OK);
    EXPECT_EQ(sht.shared.init_ref_count, 1);

    EXPECT_EQ(cfn_sal_hum_sensor_deinit(&sht.hum), CFN_HAL_ERROR_OK);
    EXPECT_EQ(sht.shared.init_ref_count, 0);
    EXPECT_FALSE(sht.shared.hw_initialized);
}

TEST_F(Sht40Test, Measurement)
{
    cfn_sal_temp_sensor_init(&sht.temp);

    float temp = 0.0f;
    (void) cfn_sal_temp_sensor_read_celsius(&sht.temp, &temp);
}

TEST_F(Sht40Test, Getters)
{
    EXPECT_EQ(cfn_sal_sht40_get_temp(&sht), &sht.temp);
    EXPECT_EQ(cfn_sal_sht40_get_hum(&sht), &sht.hum);
    EXPECT_EQ(cfn_sal_sht40_get_temp(NULL), nullptr);
}
