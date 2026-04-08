/**
 * @file cfn_sal_test_lsm6ds3.cpp
 * @brief Unit tests for the LSM6DS3 combined sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/combined_sensor/lsm6ds3/cfn_sal_lsm6ds3.h"
#include "cfn_hal_i2c.h"
#include "cfn_hal_spi.h"

class Lsm6ds3Test : public ::testing::Test
{
  protected:
    cfn_sal_lsm6ds3_t    lsm{};
    cfn_sal_phy_t        phy_i2c{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    void SetUp() override
    {
        memset(&lsm, 0, sizeof(lsm));

        /* Setup I2C Mock */
        memset(&mock_i2c, 0, sizeof(mock_i2c));
        memset(&mock_i2c_api, 0, sizeof(mock_i2c_api));

        mock_i2c_api.base.init = [](cfn_hal_driver_t *d) -> cfn_hal_error_code_t { return CFN_HAL_ERROR_OK; };
        mock_i2c_api.mem_read =
            [](cfn_hal_i2c_t *d, const cfn_hal_i2c_mem_transaction_t *xfr, uint32_t t) -> cfn_hal_error_code_t
        {
            if (xfr->mem_addr == 0x0F)
            {
                xfr->data[0] = 0x69;
            } // WHO_AM_I
            return CFN_HAL_ERROR_OK;
        };
        mock_i2c_api.mem_write = [](cfn_hal_i2c_t                       *d,
                                    const cfn_hal_i2c_mem_transaction_t *xfr,
                                    uint32_t t) -> cfn_hal_error_code_t { return CFN_HAL_ERROR_OK; };

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);
        i2c_dev.i2c      = &mock_i2c;
        i2c_dev.address  = 0x6A;

        phy_i2c.instance = &i2c_dev;
        phy_i2c.type     = CFN_HAL_PERIPHERAL_TYPE_I2C;

        cfn_sal_lsm6ds3_construct(&lsm, &phy_i2c);
    }
};

TEST_F(Lsm6ds3Test, Lifecycle)
{
    EXPECT_EQ(cfn_sal_accel_init(&lsm.accel), CFN_HAL_ERROR_OK);
    EXPECT_EQ(lsm.combined_state.init_ref_count, 1);
    EXPECT_TRUE(lsm.combined_state.hw_initialized);

    EXPECT_EQ(cfn_sal_gyro_sensor_init(&lsm.gyro), CFN_HAL_ERROR_OK);
    EXPECT_EQ(lsm.combined_state.init_ref_count, 2);

    EXPECT_EQ(cfn_sal_accel_deinit(&lsm.accel), CFN_HAL_ERROR_OK);
    EXPECT_EQ(lsm.combined_state.init_ref_count, 1);

    EXPECT_EQ(cfn_sal_gyro_sensor_deinit(&lsm.gyro), CFN_HAL_ERROR_OK);
    EXPECT_EQ(lsm.combined_state.init_ref_count, 0);
    EXPECT_FALSE(lsm.combined_state.hw_initialized);
}
