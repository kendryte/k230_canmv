/* Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_IOMUX_FPIOA_H__
#define _MACHINE_IOMUX_FPIOA_H__

#include "py/obj.h"

extern const mp_obj_type_t machine_fpioa_type;

typedef struct _machine_fpioa_obj_t {
    mp_obj_base_t base;
} machine_fpioa_obj_t;

enum en_func_def_ {
    GPIO0 ,
    GPIO1 ,
    GPIO2 ,
    GPIO3 ,
    GPIO4 ,
    GPIO5 ,
    GPIO6 ,
    GPIO7 ,
    GPIO8 ,
    GPIO9 ,
    GPIO10,
    GPIO11,
    GPIO12,
    GPIO13,
    GPIO14,
    GPIO15,
    GPIO16,
    GPIO17,
    GPIO18,
    GPIO19,
    GPIO20,
    GPIO21,
    GPIO22,
    GPIO23,
    GPIO24,
    GPIO25,
    GPIO26,
    GPIO27,
    GPIO28,
    GPIO29,
    GPIO30,
    GPIO31,
    GPIO32,
    GPIO33,
    GPIO34,
    GPIO35,
    GPIO36,
    GPIO37,
    GPIO38,
    GPIO39,
    GPIO40,
    GPIO41,
    GPIO42,
    GPIO43,
    GPIO44,
    GPIO45,
    GPIO46,
    GPIO47,
    GPIO48,
    GPIO49,
    GPIO50,
    GPIO51,
    GPIO52,
    GPIO53,
    GPIO54,
    GPIO55,
    GPIO56,
    GPIO57,
    GPIO58,
    GPIO59,
    GPIO60,
    GPIO61,
    GPIO62,
    GPIO63,
    BOOT0,
    BOOT1,
    CI0,
    CI1,
    CI2,
    CI3,
    CO0,
    CO1,
    CO2,
    CO3,
    DI0,
    DI1,
    DI2,
    DI3,
    DO0,
    DO1,
    DO2,
    DO3,
    HSYNC0,
    HSYNC1,
    IIC0_SCL,
    IIC0_SDA,
    IIC1_SCL,
    IIC1_SDA,
    IIC2_SCL,
    IIC2_SDA,
    IIC3_SCL,
    IIC3_SDA,
    IIC4_SCL,
    IIC4_SDA,
    IIS_CLK,
    IIS_D_IN0,
    IIS_D_IN1,
    IIS_D_OUT0,
    IIS_D_OUT1,
    IIS_WS,
    JTAG_RST,
    JTAG_TCK,
    JTAG_TDI,
    JTAG_TDO,
    JTAG_TMS,
    M_CLK1,
    M_CLK2,
    M_CLK3,
    MMC1_CLK,
    MMC1_CMD,
    MMC1_D0,
    MMC1_D1,
    MMC1_D2,
    MMC1_D3,
    OSPI_CLK,
    OSPI_CS,
    OSPI_D0,
    OSPI_D1,
    OSPI_D2,
    OSPI_D3,
    OSPI_D4,
    OSPI_D5,
    OSPI_D6,
    OSPI_D7,
    OSPI_DQS,
    PDM_IN0,
    PDM_IN1,
    PDM_IN2,
    PDM_IN3,
    PULSE_CNTR0,
    PULSE_CNTR1,
    PULSE_CNTR2,
    PULSE_CNTR3,
    PULSE_CNTR4,
    PULSE_CNTR5,
    PWM0,
    PWM1,
    PWM2,
    PWM3,
    PWM4,
    PWM5,
    QSPI0_CLK,
    QSPI0_CS0,
    QSPI0_CS1,
    QSPI0_CS2,
    QSPI0_CS3,
    QSPI0_CS4,
    QSPI0_D0,
    QSPI0_D1,
    QSPI0_D2,
    QSPI0_D3,
    QSPI1_CLK,
    QSPI1_CS0,
    QSPI1_CS1,
    QSPI1_CS2,
    QSPI1_CS3,
    QSPI1_CS4,
    QSPI1_D0,
    QSPI1_D1,
    QSPI1_D2,
    QSPI1_D3,
    SPI2AXI_CK,
    SPI2AXI_CS,
    SPI2AXI_DI,
    SPI2AXI_DO,
    UART0_RXD,
    UART0_TXD,
    UART1_CTS,
    UART1_RTS,
    UART1_RXD,
    UART1_TXD,
    UART2_CTS,
    UART2_RTS,
    UART2_RXD,
    UART2_TXD,
    UART3_CTS,
    UART3_DE,
    UART3_RE,
    UART3_RTS,
    UART3_RXD,
    UART3_TXD,
    UART4_RXD,
    UART4_TXD,
    PDM_CLK,
    VSYNC0,
    VSYNC1,
    CTRL_IN_3D,
    CTRL_O1_3D,
    CTRL_O2_3D,    
    TEST_PIN0 ,
    TEST_PIN1 ,
    TEST_PIN2 ,
    TEST_PIN3 ,
    TEST_PIN4 ,
    TEST_PIN5 ,
    TEST_PIN6 ,
    TEST_PIN7 ,
    TEST_PIN8 ,
    TEST_PIN9 ,
    TEST_PIN10,
    TEST_PIN11,
    TEST_PIN12,
    TEST_PIN13,
    TEST_PIN14,
    TEST_PIN15,
    TEST_PIN16,
    TEST_PIN17,
    TEST_PIN18,
    TEST_PIN19,
    TEST_PIN20,
    TEST_PIN21,
    TEST_PIN22,
    TEST_PIN23,
    TEST_PIN24,
    TEST_PIN25,
    TEST_PIN26,
    TEST_PIN27,
    TEST_PIN28,
    TEST_PIN29,
    TEST_PIN30,
    TEST_PIN31,
    FUNC_MAX,
};
#pragma pack (1)
struct st_func_describe{
    enum en_func_def_ func;
    char *name ;
    char *describe_str;
};
struct st_pin_func{
    uint8_t func0;
    uint8_t func1;
    uint8_t func2;
    uint8_t func3;
    uint8_t func4;
};
struct st_iomux_reg{
    union{
        struct {
            uint32_t st:1;//bit 0 输入施密特触发器控制使能
            uint32_t ds:4; //bit 4-1 驱动电流控制：
            uint32_t pd:1;//bit 5 下拉；
            uint32_t pu:1;//bit 6 
            uint32_t oe:1; //bit 7 out 
            uint32_t ie:1; //bit 8 input
            uint32_t msc:1;// bit 9 电压选择；
            uint32_t sl:1;//bit 10  输出翻转率控制使能（双电压PAD无此控制端
            uint32_t io_sel:3; //bit 13-11 func 选择
            uint32_t rsv:17; // bit 30-14
            uint32_t di:1; // bit 31 当前PAD输入到芯片内部的数据(即PAD的C端)
        }bit;
        uint32_t value;
    }u;
};

#pragma pack ()
#endif
