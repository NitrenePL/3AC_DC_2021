#include "oled.h"
#include "oledfont.h"
#include "board.h"

#define OLED_I2C_BASE       myI2C0_BASE
#define OLED_I2C_SDA_GPIO   myI2C0_I2CSDA_GPIO
#define OLED_I2C_SCL_GPIO   myI2C0_I2CSCL_GPIO
#define OLED_SLAVE_ADDR     0x3CU
#define OLED_I2C_TIMEOUT    100000UL
#define OLED_I2C_CHUNK_SIZE 15U
#define OLED_WIDTH          128U
#define OLED_PAGES          8U

static uint16_t OLED_I2C_WaitStopReady(void)
{
    uint32_t timeout = OLED_I2C_TIMEOUT;

    while (I2C_getStopConditionStatus(OLED_I2C_BASE) && (--timeout > 0UL))
    {
    }

    return (timeout > 0UL) ? 0U : 1U;
}

static uint16_t OLED_I2C_WaitBusFree(void)
{
    uint32_t timeout = OLED_I2C_TIMEOUT;

    while (I2C_isBusBusy(OLED_I2C_BASE) && (--timeout > 0UL))
    {
    }

    return (timeout > 0UL) ? 0U : 1U;
}

static uint16_t OLED_I2C_WaitFrameDone(void)
{
    uint32_t timeout = OLED_I2C_TIMEOUT;
    uint16_t status;

    do
    {
        status = I2C_getStatus(OLED_I2C_BASE);
        if ((status & (I2C_STS_NO_ACK | I2C_STS_ARB_LOST)) != 0U)
        {
            I2C_clearStatus(OLED_I2C_BASE, I2C_STS_NO_ACK | I2C_STS_ARB_LOST);
            return 1U;
        }
    } while (I2C_getStopConditionStatus(OLED_I2C_BASE) && (--timeout > 0UL));

    return (timeout > 0UL) ? 0U : 1U;
}

static void OLED_I2C_BusRelease(void)
{
    uint16_t i;

    I2C_disableModule(OLED_I2C_BASE);

    GPIO_setPinConfig(GPIO_26_GPIO26);
    GPIO_setPinConfig(GPIO_27_GPIO27);
    GPIO_setPadConfig(OLED_I2C_SDA_GPIO, GPIO_PIN_TYPE_OD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setPadConfig(OLED_I2C_SCL_GPIO, GPIO_PIN_TYPE_OD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(OLED_I2C_SDA_GPIO, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(OLED_I2C_SCL_GPIO, GPIO_QUAL_ASYNC);
    GPIO_setDirectionMode(OLED_I2C_SDA_GPIO, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(OLED_I2C_SCL_GPIO, GPIO_DIR_MODE_OUT);

    GPIO_writePin(OLED_I2C_SDA_GPIO, 1U);
    GPIO_writePin(OLED_I2C_SCL_GPIO, 1U);
    DEVICE_DELAY_US(5);

    for (i = 0U; i < 9U; i++)
    {
        GPIO_writePin(OLED_I2C_SCL_GPIO, 0U);
        DEVICE_DELAY_US(5);
        GPIO_writePin(OLED_I2C_SCL_GPIO, 1U);
        DEVICE_DELAY_US(5);
    }

    GPIO_writePin(OLED_I2C_SDA_GPIO, 0U);
    DEVICE_DELAY_US(5);
    GPIO_writePin(OLED_I2C_SCL_GPIO, 1U);
    DEVICE_DELAY_US(5);
    GPIO_writePin(OLED_I2C_SDA_GPIO, 1U);
    DEVICE_DELAY_US(5);

    GPIO_setPinConfig(myI2C0_I2CSDA_PIN_CONFIG);
    GPIO_setPinConfig(myI2C0_I2CSCL_PIN_CONFIG);
    GPIO_setPadConfig(OLED_I2C_SDA_GPIO, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setPadConfig(OLED_I2C_SCL_GPIO, GPIO_PIN_TYPE_STD | GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(OLED_I2C_SDA_GPIO, GPIO_QUAL_ASYNC);
    GPIO_setQualificationMode(OLED_I2C_SCL_GPIO, GPIO_QUAL_ASYNC);

    myI2C0_init();
}

static void OLED_I2C_Recover(void)
{
    I2C_sendStopCondition(OLED_I2C_BASE);
    I2C_disableModule(OLED_I2C_BASE);
    OLED_I2C_BusRelease();
}

static uint16_t OLED_I2C_WriteFrame(uint8_t control, const uint8_t *data, uint16_t length)
{
    uint16_t i;

    if ((data == 0U) || (length == 0U) || (length > OLED_I2C_CHUNK_SIZE))
    {
        return 1U;
    }

    if ((OLED_I2C_WaitStopReady() != 0U) || (OLED_I2C_WaitBusFree() != 0U))
    {
        OLED_I2C_Recover();
        return 1U;
    }

    I2C_clearStatus(OLED_I2C_BASE,
                    I2C_STS_NO_ACK | I2C_STS_ARB_LOST |
                    I2C_STS_REG_ACCESS_RDY | I2C_STS_STOP_CONDITION |
                    I2C_STS_BYTE_SENT);
    I2C_setTargetAddress(OLED_I2C_BASE, OLED_SLAVE_ADDR);
    I2C_setDataCount(OLED_I2C_BASE, length + 1U);
    I2C_setConfig(OLED_I2C_BASE, I2C_CONTROLLER_SEND_MODE);

    I2C_putData(OLED_I2C_BASE, control);
    for (i = 0U; i < length; i++)
    {
        I2C_putData(OLED_I2C_BASE, data[i]);
    }

    I2C_sendStartCondition(OLED_I2C_BASE);
    I2C_sendStopCondition(OLED_I2C_BASE);

    if (OLED_I2C_WaitFrameDone() != 0U)
    {
        OLED_I2C_Recover();
        return 1U;
    }

    return 0U;
}

static void OLED_WriteBuffer(uint8_t control, const uint8_t *data, uint16_t length)
{
    uint16_t chunk;

    while (length > 0U)
    {
        chunk = (length > OLED_I2C_CHUNK_SIZE) ? OLED_I2C_CHUNK_SIZE : length;
        if (OLED_I2C_WriteFrame(control, data, chunk) != 0U)
        {
            return;
        }
        data += chunk;
        length -= chunk;
    }
}

static void OLED_WriteCommandBuffer(const uint8_t *commands, uint16_t length)
{
    OLED_WriteBuffer(0x00U, commands, length);
}

static void OLED_WriteDataBuffer(const uint8_t *data, uint16_t length)
{
    OLED_WriteBuffer(0x40U, data, length);
}

void I2C_Write_Byte(uint8_t control_byte, uint8_t data)
{
    OLED_I2C_WriteFrame(control_byte, &data, 1U);
}

void Write_IIC_Command(unsigned char i2c_cmd)
{
    uint8_t command = (uint8_t)i2c_cmd;
    OLED_WriteCommandBuffer(&command, 1U);
}

void Write_IIC_Data(unsigned char i2c_data)
{
    uint8_t data = (uint8_t)i2c_data;
    OLED_WriteDataBuffer(&data, 1U);
}

void OLED_WR_Byte(unsigned char dat, unsigned char cmd)
{
    if (cmd != 0U)
    {
        Write_IIC_Data(dat);
    }
    else
    {
        Write_IIC_Command(dat);
    }
}

void OLED_Set_Pos(unsigned char x, unsigned char y)
{
    uint8_t commands[3];

    commands[0] = (uint8_t)(0xB0U + y);
    commands[1] = (uint8_t)(((x & 0xF0U) >> 4U) | 0x10U);
    commands[2] = (uint8_t)(x & 0x0FU);
    OLED_WriteCommandBuffer(commands, 3U);
}

void OLED_Display_On(void)
{
    static const uint8_t commands[] = {0x8DU, 0x14U, 0xAFU};
    OLED_WriteCommandBuffer(commands, sizeof(commands));
}

void OLED_Display_Off(void)
{
    static const uint8_t commands[] = {0x8DU, 0x10U, 0xAEU};
    OLED_WriteCommandBuffer(commands, sizeof(commands));
}

void OLED_Clear(void)
{
    uint8_t page;
    uint8_t i;
    uint8_t clearData[OLED_I2C_CHUNK_SIZE] = {0U};

    for (page = 0U; page < OLED_PAGES; page++)
    {
        OLED_Set_Pos(0U, page);
        for (i = 0U; i < 8U; i++)
        {
            OLED_WriteDataBuffer(clearData, sizeof(clearData));
        }
        OLED_WriteDataBuffer(clearData, 8U);
    }
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    uint8_t c;

    if ((chr < ' ') || (chr > '~'))
    {
        chr = ' ';
    }

    c = (uint8_t)(chr - ' ');

    if (x > (OLED_WIDTH - 1U))
    {
        x = 0U;
        y = (uint8_t)(y + 2U);
    }

    if (Char_Size == 16U)
    {
        OLED_Set_Pos(x, y);
        OLED_WriteDataBuffer((const uint8_t *)&F8X16[c * 16U], 8U);
        OLED_Set_Pos(x, (uint8_t)(y + 1U));
        OLED_WriteDataBuffer((const uint8_t *)&F8X16[c * 16U + 8U], 8U);
    }
    else
    {
        OLED_Set_Pos(x, y);
        OLED_WriteDataBuffer((const uint8_t *)F6x8[c], 6U);
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t Char_Size)
{
    uint8_t j = 0U;
    uint8_t step = (Char_Size == 16U) ? 8U : 6U;

    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x = (uint8_t)(x + step);
        if (x > 120U)
        {
            x = 0U;
            y = (uint8_t)(y + ((Char_Size == 16U) ? 2U : 1U));
        }
        j++;
    }
}

static uint32_t OLED_Pow(uint32_t base, uint8_t exp)
{
    uint32_t result = 1U;

    while (exp-- > 0U)
    {
        result *= base;
    }

    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size2)
{
    uint8_t i;

    for (i = 0U; i < len; i++)
    {
        OLED_ShowChar(x, y,
                      (uint8_t)((num / OLED_Pow(10U, (uint8_t)(len - i - 1U))) % 10U + '0'),
                      size2);
        x = (uint8_t)(x + ((size2 == 16U) ? 8U : 6U));
    }
}

void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
    OLED_Set_Pos(x, y);
    OLED_WriteDataBuffer((const uint8_t *)&Hzk[no][0], 16U);
    OLED_Set_Pos(x, (uint8_t)(y + 1U));
    OLED_WriteDataBuffer((const uint8_t *)&Hzk[no][16], 16U);
}

void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[])
{
    uint8_t y;
    uint8_t width = (uint8_t)(x1 - x0);
    uint16_t index = 0U;

    for (y = y0; y < y1; y++)
    {
        OLED_Set_Pos(x0, y);
        OLED_WriteDataBuffer((uint8_t *)&BMP[index], width);
        index = (uint16_t)(index + width);
    }
}

void OLED_Init(void)
{
    static const uint8_t initCommands[] = {
        0xAEU,       // Display off
        0x00U, 0x10U,
        0x40U,
        0xB0U,
        0x81U, 0xFFU,
        0xA1U,
        0xA6U,
        0xA8U, 0x3FU,
        0xC8U,
        0xD3U, 0x00U,
        0xD5U, 0x80U,
        0xD9U, 0xF1U,
        0xDAU, 0x12U,
        0xDBU, 0x30U,
        0x8DU, 0x14U,
        0xAFU
    };

    DEVICE_DELAY_US(100000);
    OLED_I2C_BusRelease();
    OLED_WriteCommandBuffer(initCommands, sizeof(initCommands));
    OLED_Clear();
}
