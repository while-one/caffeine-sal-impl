/**
 * @file cfn_sal_test_opt4048.cpp
 * @brief Unit tests for the OPT4048 combined sensor implementation.
 */

#include <gtest/gtest.h>
#include "devices/combined_sensor/opt4048/cfn_sal_opt4048.h"
#include "cfn_hal_i2c.h"

class Opt4048Test : public ::testing::Test
{
  protected:
    cfn_sal_opt4048_t    opt{};
    cfn_sal_phy_t        phy{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t        mock_i2c{};
    cfn_hal_i2c_api_t    mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    void SetUp() override
    {
        memset(&opt, 0, sizeof(opt));
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
            if (mem_xfr->mem_addr == 0x11 && mem_xfr->size == 2) /* ID Register */
            {
                mem_xfr->data[0] = 0x08; /* MSB */
                mem_xfr->data[1] = 0x21; /* LSB */
            }
            else if (mem_xfr->mem_addr == 0x0A && mem_xfr->size == 2) /* CONFIG */
            {
                mem_xfr->data[0] = 0x00;
                mem_xfr->data[1] = 0x00;
            }
            else if (mem_xfr->mem_addr == 0x00 && mem_xfr->size == 16) /* All Channels */
            {
                /* Construct mock data.
                 * Example: Mantissa = 1000 (0x3E8), Exponent = 0.
                 * Value = 1000.
                 * MSB = (Exp << 12) | (Mantissa >> 8) = 0x0003
                 * LSB = (Mantissa & 0xFF) << 8 = 0xE800 (ignoring CNT/CRC)
                 */
                uint16_t mock_msb = 0x0003;
                uint16_t mock_lsb = 0xE800;

                for (int i = 0; i < 4; i++)
                {
                    mem_xfr->data[(i * 4) + 0] = (mock_msb >> 8) & 0xFF;
                    mem_xfr->data[(i * 4) + 1] = mock_msb & 0xFF;
                    mem_xfr->data[(i * 4) + 2] = (mock_lsb >> 8) & 0xFF;
                    mem_xfr->data[(i * 4) + 3] = mock_lsb & 0xFF;
                }
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
        i2c_dev.address = CFN_SAL_OPT4048_ADDR_DEFAULT;

        phy.instance    = &i2c_dev;
        phy.type        = CFN_HAL_PERIPHERAL_TYPE_I2C;

        cfn_sal_opt4048_construct(&opt, &phy, NULL);
    }
};

TEST_F(Opt4048Test, Lifecycle)
{
    EXPECT_EQ(cfn_sal_light_sensor_init(&opt.light), CFN_HAL_ERROR_OK);
    EXPECT_EQ(opt.combined_state.init_ref_count, 1);
    EXPECT_TRUE(opt.combined_state.hw_initialized);

    EXPECT_EQ(cfn_sal_color_sensor_init(&opt.color), CFN_HAL_ERROR_OK);
    EXPECT_EQ(opt.combined_state.init_ref_count, 2);

    EXPECT_EQ(cfn_sal_light_sensor_deinit(&opt.light), CFN_HAL_ERROR_OK);
    EXPECT_EQ(opt.combined_state.init_ref_count, 1);

    EXPECT_EQ(cfn_sal_color_sensor_deinit(&opt.color), CFN_HAL_ERROR_OK);
    EXPECT_EQ(opt.combined_state.init_ref_count, 0);
    EXPECT_FALSE(opt.combined_state.hw_initialized);
}

TEST_F(Opt4048Test, MathDecoding)
{
    cfn_sal_color_sensor_init(&opt.color);

    uint32_t ch0, ch1, ch2, ch3;
    EXPECT_EQ(cfn_sal_color_sensor_read_raw_channels(&opt.color, &ch0, &ch1, &ch2, &ch3), CFN_HAL_ERROR_OK);

    /* With our mock data: Mantissa=1000, Exponent=0 */
    EXPECT_EQ(ch0, 1000);
    EXPECT_EQ(ch1, 1000);
    EXPECT_EQ(ch2, 1000);
    EXPECT_EQ(ch3, 1000);

    float lux = 0.0f;
    EXPECT_EQ(cfn_sal_light_sensor_read_lux(&opt.light, &lux), CFN_HAL_ERROR_OK);
    /* Lux is CH1 * 0.00215 -> 1000 * 0.00215 = 2.15 */
    EXPECT_FLOAT_EQ(lux, 2.15f);

    float x, y, z;
    EXPECT_EQ(cfn_sal_color_sensor_read_xyz(&opt.color, &x, &y, &z), CFN_HAL_ERROR_OK);
    EXPECT_FLOAT_EQ(x, 2.15f);
    EXPECT_FLOAT_EQ(y, 2.15f);
    EXPECT_FLOAT_EQ(z, 2.15f);

    float cct = 0.0f;
    EXPECT_EQ(cfn_sal_color_sensor_read_cct(&opt.color, &cct), CFN_HAL_ERROR_OK);
    /* For x=y=z, x_chrom = 1/3, y_chrom = 1/3.
       n = (0.333333 - 0.3320) / (0.1858 - 0.333333)
         = 0.001333 / -0.147533
         = -0.009035
       cct = 449*n^3 + 3525*n^2 + 6823.3*n + 5520.33
           = roughly 5520 - 61 + ... = ~5458 K
    */
    /* We just ensure it calculates something without crashing and provides a float. */
    EXPECT_GT(cct, 5000.0f);
    EXPECT_LT(cct, 6000.0f);
}
