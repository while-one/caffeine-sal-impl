#include <gtest/gtest.h>
#include "cfn_hal_i2c_mock.h"
#include "cfn_sal_pcf8574.h"

using namespace testing;

class PCF8574Test : public Test
{
  protected:
    void SetUp() override
    {
        i2c_mock        = new StrictMock<I2CMock>();

        // Setup phy
        i2c_dev.i2c     = (cfn_hal_i2c_t *) i2c_mock; // In mock land, the driver pointer IS the mock object
        i2c_dev.address = 0x20;

        phy.type        = CFN_HAL_PERIPHERAL_TYPE_I2C;
        phy.instance    = &i2c_dev;

        cfn_sal_pcf8574_construct(&pcf, &phy);
    }

    void TearDown() override
    {
        cfn_sal_pcf8574_destruct(&pcf);
        delete i2c_mock;
    }

    StrictMock<I2CMock> *i2c_mock;
    cfn_hal_i2c_device_t i2c_dev;
    cfn_sal_phy_t        phy;
    cfn_sal_pcf8574_t    pcf;
};

TEST_F(PCF8574Test, InitWritesAllHigh)
{
    uint8_t expected_data = 0xFF;
    EXPECT_CALL(*i2c_mock, master_write(_, 0x20, Pointee(expected_data), 1, _)).WillOnce(Return(CFN_HAL_ERROR_OK));

    EXPECT_EQ(cfn_hal_gpio_init(&pcf.hal_gpio), CFN_HAL_ERROR_OK);
}

TEST_F(PCF8574Test, WritePinResetClearsBit)
{
    // First init to get high state
    EXPECT_CALL(*i2c_mock, master_write(_, _, _, _, _)).WillRepeatedly(Return(CFN_HAL_ERROR_OK));
    cfn_hal_gpio_init(&pcf.hal_gpio);

    // PCF8574: P0 is Bit 0. Reset should write 0xFE (1111 1110)
    uint8_t expected_data = 0xFE;
    EXPECT_CALL(*i2c_mock, master_write(_, 0x20, Pointee(expected_data), 1, _)).WillOnce(Return(CFN_HAL_ERROR_OK));

    EXPECT_EQ(cfn_hal_gpio_pin_write(&pcf.hal_gpio, CFN_HAL_BIT(0), CFN_HAL_GPIO_PIN_RESET), CFN_HAL_ERROR_OK);
}

TEST_F(PCF8574Test, ReadPinReadsFromBus)
{
    EXPECT_CALL(*i2c_mock, master_write(_, _, _, _, _)).WillRepeatedly(Return(CFN_HAL_ERROR_OK));
    cfn_hal_gpio_init(&pcf.hal_gpio);

    uint8_t bus_data = 0xAA; // 1010 1010
    EXPECT_CALL(*i2c_mock, master_read(_, 0x20, _, 1, _))
        .WillOnce(DoAll(SetArgPointee<2>(bus_data), Return(CFN_HAL_ERROR_OK)));

    cfn_hal_gpio_pin_state_t state;
    EXPECT_EQ(cfn_hal_gpio_pin_read(&pcf.hal_gpio, CFN_HAL_BIT(1), &state), CFN_HAL_ERROR_OK);
    EXPECT_EQ(state, CFN_HAL_GPIO_PIN_SET); // Bit 1 is 1

    EXPECT_CALL(*i2c_mock, master_read(_, 0x20, _, 1, _))
        .WillOnce(DoAll(SetArgPointee<2>(bus_data), Return(CFN_HAL_ERROR_OK)));
    EXPECT_EQ(cfn_hal_gpio_pin_read(&pcf.hal_gpio, CFN_HAL_BIT(0), &state), CFN_HAL_ERROR_OK);
    EXPECT_EQ(state, CFN_HAL_GPIO_PIN_RESET); // Bit 0 is 0
}

TEST_F(PCF8574Test, WriteToInputFails)
{
    EXPECT_CALL(*i2c_mock, master_write(_, _, _, _, _)).WillRepeatedly(Return(CFN_HAL_ERROR_OK));
    cfn_hal_gpio_init(&pcf.hal_gpio);

    // Configure Pin 4 as input
    cfn_hal_gpio_pin_config_t cfg = { .pin_mask = CFN_HAL_BIT(4), .mode = CFN_HAL_GPIO_CONFIG_MODE_INPUT };
    cfn_hal_gpio_pin_config(&pcf.hal_gpio, &cfg);

    // Try to write to it - should fail
    EXPECT_EQ(cfn_hal_gpio_pin_write(&pcf.hal_gpio, CFN_HAL_BIT(4), CFN_HAL_GPIO_PIN_RESET), CFN_HAL_ERROR_BAD_PARAM);
}
