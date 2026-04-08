/**
 * @file cfn_sal_test_vcnl4040.cpp
 * @brief Unit tests for the VCNL4040 combined sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/combined_sensor/vcnl4040/cfn_sal_vcnl4040.h"
#include "cfn_hal_i2c.h"

class Vcnl4040Test : public ::testing::Test
{
  protected:
    cfn_sal_vcnl4040_t   vcnl{};
    cfn_sal_phy_t        phy{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    void SetUp() override
    {
        memset(&vcnl, 0, sizeof(vcnl));
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
            [](cfn_hal_i2c_t *d, const cfn_hal_i2c_mem_transaction_t *mem_xfr, uint32_t timeout) -> cfn_hal_error_code_t
        {
            (void) d;
            (void) timeout;
            if (mem_xfr->mem_addr == 0x0C && mem_xfr->size == 2) /* ID Register */
            {
                mem_xfr->data[0] = 0x86; /* LSB */
                mem_xfr->data[1] = 0x01; /* MSB */
            }
            else if (mem_xfr->mem_addr == 0x09 && mem_xfr->size == 2) /* ALS_DATA */
            {
                mem_xfr->data[0] = 0xE8; /* LSB */
                mem_xfr->data[1] = 0x03; /* MSB -> 1000 raw -> 100.0 lux */
            }
            else if (mem_xfr->mem_addr == 0x08 && mem_xfr->size == 2) /* PS_DATA */
            {
                mem_xfr->data[0] = 0x55; /* LSB */
                mem_xfr->data[1] = 0xAA; /* MSB -> dummy */
            }
            return CFN_HAL_ERROR_OK;
        };
        mock_i2c_api.mem_write =
            [](cfn_hal_i2c_t *d, const cfn_hal_i2c_mem_transaction_t *mem_xfr, uint32_t timeout) -> cfn_hal_error_code_t
        {
            (void) d;
            (void) timeout;
            (void) mem_xfr;
            return CFN_HAL_ERROR_OK;
        };

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);

        i2c_dev.i2c     = &mock_i2c;
        i2c_dev.address = CFN_SAL_VCNL4040_ADDR_DEFAULT;

        phy.instance    = &i2c_dev;
        phy.type        = CFN_HAL_PERIPHERAL_TYPE_I2C;

        cfn_sal_vcnl4040_construct(&vcnl, &phy, NULL);
    }
};

TEST_F(Vcnl4040Test, Lifecycle)
{
    EXPECT_EQ(cfn_sal_light_sensor_init(&vcnl.light), CFN_HAL_ERROR_OK);
    EXPECT_EQ(vcnl.combined_state.init_ref_count, 1);
    EXPECT_TRUE(vcnl.combined_state.hw_initialized);

    EXPECT_EQ(cfn_sal_prox_sensor_init(&vcnl.prox), CFN_HAL_ERROR_OK);
    EXPECT_EQ(vcnl.combined_state.init_ref_count, 2);

    EXPECT_EQ(cfn_sal_light_sensor_deinit(&vcnl.light), CFN_HAL_ERROR_OK);
    EXPECT_EQ(vcnl.combined_state.init_ref_count, 1);

    EXPECT_EQ(cfn_sal_prox_sensor_deinit(&vcnl.prox), CFN_HAL_ERROR_OK);
    EXPECT_EQ(vcnl.combined_state.init_ref_count, 0);
    EXPECT_FALSE(vcnl.combined_state.hw_initialized);
}

TEST_F(Vcnl4040Test, Measurement)
{
    cfn_sal_light_sensor_init(&vcnl.light);

    float lux = 0.0f;
    EXPECT_EQ(cfn_sal_light_sensor_read_lux(&vcnl.light, &lux), CFN_HAL_ERROR_OK);
    EXPECT_FLOAT_EQ(lux, 100.0f);

    float prox = 0.0f;
    EXPECT_EQ(cfn_sal_prox_sensor_read_distance_mm(&vcnl.prox, &prox), CFN_HAL_ERROR_OK);
}
