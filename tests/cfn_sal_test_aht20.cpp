/**
 * @file cfn_sal_test_aht20.cpp
 * @brief Unit tests for the AHT20 combined sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/combined_sensor/aht20/cfn_sal_aht20.h"
#include "cfn_hal_i2c.h"

class Aht20Test : public ::testing::Test
{
  protected:
    cfn_sal_aht20_t      aht{};
    cfn_sal_phy_t        phy{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    void SetUp() override
    {
        memset(&aht, 0, sizeof(aht));
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
            if (xfr->rx_payload && xfr->nbr_of_rx_bytes >= 7)
            {
                /* Mock data: calibrated, ready */
                xfr->rx_payload[0] = 0x08; /* Calibrated bit */
                /* Dummy data for rest */
                memset(&xfr->rx_payload[1], 0xAA, 5);
                xfr->rx_payload[6] = 0xCC; /* Dummy CRC */
            }
            else if (xfr->rx_payload && xfr->nbr_of_rx_bytes == 1)
            {
                xfr->rx_payload[0] = 0x08; /* Always calibrated for test */
            }
            return CFN_HAL_ERROR_OK;
        };

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);

        i2c_dev.i2c     = &mock_i2c;
        i2c_dev.address = CFN_SAL_AHT20_ADDR_DEFAULT;

        phy.instance    = &i2c_dev;
        phy.type        = CFN_HAL_PERIPHERAL_TYPE_I2C;

        cfn_sal_aht20_construct(&aht, &phy);
    }
};

TEST_F(Aht20Test, Lifecycle)
{
    EXPECT_EQ(cfn_sal_temp_sensor_init(&aht.temp), CFN_HAL_ERROR_OK);
    EXPECT_EQ(aht.combined_state.init_ref_count, 1);
    EXPECT_TRUE(aht.combined_state.hw_initialized);

    EXPECT_EQ(cfn_sal_hum_sensor_init(&aht.hum), CFN_HAL_ERROR_OK);
    EXPECT_EQ(aht.combined_state.init_ref_count, 2);

    EXPECT_EQ(cfn_sal_temp_sensor_deinit(&aht.temp), CFN_HAL_ERROR_OK);
    EXPECT_EQ(aht.combined_state.init_ref_count, 1);

    EXPECT_EQ(cfn_sal_hum_sensor_deinit(&aht.hum), CFN_HAL_ERROR_OK);
    EXPECT_EQ(aht.combined_state.init_ref_count, 0);
    EXPECT_FALSE(aht.combined_state.hw_initialized);
}
