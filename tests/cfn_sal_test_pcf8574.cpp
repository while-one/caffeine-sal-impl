#include <gtest/gtest.h>
#include "cfn_hal_i2c.h"
#include "devices/gpio_expander/pcf8574/cfn_sal_pcf8574.h"

class PCF8574Test : public ::testing::Test
{
protected:
    cfn_sal_pcf8574_t pcf{};
    cfn_sal_phy_t phy{};
    cfn_hal_i2c_device_t i2c_dev{};

    cfn_hal_i2c_t mock_i2c{};
    cfn_hal_i2c_api_t mock_i2c_api{};
    cfn_hal_i2c_config_t mock_i2c_cfg{};

    static inline uint8_t last_written_byte = 0;
    static inline uint8_t next_read_byte = 0;

    void SetUp() override
    {
        memset(&pcf, 0, sizeof(pcf));
        memset(&phy, 0, sizeof(phy));
        memset(&i2c_dev, 0, sizeof(i2c_dev));
        memset(&mock_i2c, 0, sizeof(mock_i2c));
        memset(&mock_i2c_api, 0, sizeof(mock_i2c_api));
        memset(&mock_i2c_cfg, 0, sizeof(mock_i2c_cfg));

        mock_i2c_api.base.init = [](cfn_hal_driver_t *d) -> cfn_hal_error_code_t {
            (void)d;
            return CFN_HAL_ERROR_OK;
        };
        mock_i2c_api.xfr_polling = [](cfn_hal_i2c_t *d, const cfn_hal_i2c_transaction_t *xfr, uint32_t t) -> cfn_hal_error_code_t {
            (void)d;
            (void)t;
            if (xfr->tx_payload) {
                last_written_byte = xfr->tx_payload[0];
            }
            if (xfr->rx_payload) {
                xfr->rx_payload[0] = next_read_byte;
            }
            return CFN_HAL_ERROR_OK;
        };

        cfn_hal_i2c_populate(&mock_i2c, 0, nullptr, &mock_i2c_api, nullptr, &mock_i2c_cfg, nullptr, nullptr);

        i2c_dev.i2c = &mock_i2c;
        i2c_dev.address = 0x20;

        phy.instance = &i2c_dev;
        phy.type = CFN_HAL_PERIPHERAL_TYPE_I2C;

        cfn_sal_pcf8574_construct(&pcf, &phy);
    }
};

TEST_F(PCF8574Test, InitWritesAllHigh)
{
    EXPECT_EQ(cfn_hal_gpio_init(&pcf.hal_gpio), CFN_HAL_ERROR_OK);
    EXPECT_EQ(last_written_byte, 0xFF);
}

TEST_F(PCF8574Test, WritePinResetClearsBit)
{
    cfn_hal_gpio_init(&pcf.hal_gpio);
    
    // PCF8574: P0 is Bit 0. Reset should write 0xFE (1111 1110)
    EXPECT_EQ(cfn_hal_gpio_pin_write(&pcf.hal_gpio, CFN_HAL_GPIO_PIN_0, CFN_HAL_GPIO_STATE_LOW), CFN_HAL_ERROR_OK);
    EXPECT_EQ(last_written_byte, 0xFE);
}

TEST_F(PCF8574Test, ReadPinReadsFromBus)
{
    cfn_hal_gpio_init(&pcf.hal_gpio);

    next_read_byte = 0xAA; // 1010 1010
    cfn_hal_gpio_state_t state;
    EXPECT_EQ(cfn_hal_gpio_pin_read(&pcf.hal_gpio, CFN_HAL_GPIO_PIN_1, &state), CFN_HAL_ERROR_OK);
    EXPECT_EQ(state, CFN_HAL_GPIO_STATE_HIGH); // Bit 1 is 1
    
    EXPECT_EQ(cfn_hal_gpio_pin_read(&pcf.hal_gpio, CFN_HAL_GPIO_PIN_0, &state), CFN_HAL_ERROR_OK);
    EXPECT_EQ(state, CFN_HAL_GPIO_STATE_LOW); // Bit 0 is 0
}

TEST_F(PCF8574Test, WriteToInputFails)
{
    cfn_hal_gpio_init(&pcf.hal_gpio);
    
    // Configure Pin 4 as input
    cfn_hal_gpio_pin_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.pin_mask = CFN_HAL_GPIO_PIN_4;
    cfg.mode = CFN_HAL_GPIO_CONFIG_MODE_INPUT;
    
    cfn_hal_gpio_pin_config(&pcf.hal_gpio, &cfg);
    
    // Try to write to it - should fail
    EXPECT_EQ(cfn_hal_gpio_pin_write(&pcf.hal_gpio, CFN_HAL_GPIO_PIN_4, CFN_HAL_GPIO_STATE_LOW), CFN_HAL_ERROR_BAD_PARAM);
}
