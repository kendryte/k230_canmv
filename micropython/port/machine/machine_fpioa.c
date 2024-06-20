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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "sys/ioctl.h"
#include "machine_fpioa.h"
#include "py/runtime.h"
#include "py/obj.h"
#include <sys/mman.h>

#define MAX_PIN_NUM 64
#define PIN_FUNC_NUM 5
#define FUNC_ALT_PIN_NUM 4
#define TEMP_STR_LEN 128
#define IOMUX_REG_ADD 0X91105000

typedef struct {
    mp_obj_base_t base;
} machine_fpioa_obj_t;

static const machine_fpioa_obj_t machine_fpioa_obj = {{&machine_fpioa_type}};

enum en_func_def {
    GPIO0,
    GPIO1,
    GPIO2,
    GPIO3,
    GPIO4,
    GPIO5,
    GPIO6,
    GPIO7,
    GPIO8,
    GPIO9,
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
    TEST_PIN0,
    TEST_PIN1,
    TEST_PIN2,
    TEST_PIN3,
    TEST_PIN4,
    TEST_PIN5,
    TEST_PIN6,
    TEST_PIN7,
    TEST_PIN8,
    TEST_PIN9,
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

struct st_func_describe {
    enum en_func_def func;
    char *name;
    uint32_t default_cfg;
};

static const struct st_func_describe g_func_describ_array[] = {
    { BOOT0, "BOOT0", 0x101 },
    { BOOT1, "BOOT1", 0x101 },
    { CI0, "RESV", 0x101 },
    { CI1, "RESV", 0x101 },
    { CI2, "RESV", 0x101 },
    { CI3, "RESV", 0x101 },
    { CO0, "RESV", 0x08E },
    { CO1, "RESV", 0x08E },
    { CO2, "RESV", 0x08E },
    { CO3, "RESV", 0x08E },
    { DI0, "RESV", 0x101 },
    { DI1, "RESV", 0x101 },
    { DI2, "RESV", 0x101 },
    { DI3, "RESV", 0x101 },
    { DO0, "RESV", 0x08E },
    { DO1, "RESV", 0x08E },
    { DO2, "RESV", 0x08E },
    { DO3, "RESV", 0x08E },
    { HSYNC0, "HSYNC0", 0x08E },
    { HSYNC1, "HSYNC1", 0x08E },
    { IIC0_SCL, "IIC0_SCL", 0x18F },
    { IIC0_SDA, "IIC0_SDA", 0x18F },
    { IIC1_SCL, "IIC1_SCL", 0x18F },
    { IIC1_SDA, "IIC1_SDA", 0x18F },
    { IIC2_SCL, "IIC2_SCL", 0x18F },
    { IIC2_SDA, "IIC2_SDA", 0x18F },
    { IIC3_SCL, "IIC3_SCL", 0x18F },
    { IIC3_SDA, "IIC3_SDA", 0x18F },
    { IIC4_SCL, "IIC4_SCL", 0x18F },
    { IIC4_SDA, "IIC4_SDA", 0x18F },
    { IIS_CLK, "IIS_CLK", 0x08E },
    { IIS_D_IN0, "IIS_D_IN0", 0x10F },
    { IIS_D_IN1, "IIS_D_IN1", 0x10F },
    { IIS_D_OUT0, "IIS_D_OUT0", 0x08E },
    { IIS_D_OUT1, "IIS_D_OUT1", 0x08E },
    { IIS_WS, "IIS_WS", 0x08E },
    { JTAG_RST, "JTAG_RST", 0x141 },
    { JTAG_TCK, "JTAG_TCK", 0x121 },
    { JTAG_TDI, "JTAG_TDI", 0x121 },
    { JTAG_TDO, "JTAG_TDO", 0x08E },
    { JTAG_TMS, "JTAG_TMS", 0x121 },
    { M_CLK1, "M_CLK1", 0x08E },
    { M_CLK2, "M_CLK2", 0x08E },
    { M_CLK3, "M_CLK3", 0x08E },
    { MMC1_CLK, "MMC1_CLK", 0x08E },
    { MMC1_CMD, "MMC1_CMD", 0x1CF },
    { MMC1_D0, "MMC1_D0", 0x1CF },
    { MMC1_D1, "MMC1_D1", 0x1CF },
    { MMC1_D2, "MMC1_D2", 0x1CF },
    { MMC1_D3, "MMC1_D3", 0x1CF },
    { OSPI_CLK, "OSPI_CLK", 0x08E },
    { OSPI_CS, "OSPI_CS", 0x0CE },
    { OSPI_D0, "OSPI_D0", 0x18F },
    { OSPI_D1, "OSPI_D1", 0x18F },
    { OSPI_D2, "OSPI_D2", 0x18F },
    { OSPI_D3, "OSPI_D3", 0x18F },
    { OSPI_D4, "OSPI_D4", 0x18F },
    { OSPI_D5, "OSPI_D5", 0x18F },
    { OSPI_D6, "OSPI_D6", 0x18F },
    { OSPI_D7, "OSPI_D7", 0x18F },
    { OSPI_DQS, "OSPI_DQS", 0x101 },
    { PDM_IN0, "PDM_IN0", 0x101 },
    { PDM_IN1, "PDM_IN1", 0x101 },
    { PDM_IN2, "PDM_IN2", 0x101 },
    { PDM_IN3, "PDM_IN3", 0x101 },
    { PULSE_CNTR0, "PULSE_CNTR0", 0x101 },
    { PULSE_CNTR1, "PULSE_CNTR1", 0x101 },
    { PULSE_CNTR2, "PULSE_CNTR2", 0x101 },
    { PULSE_CNTR3, "PULSE_CNTR3", 0x101 },
    { PULSE_CNTR4, "PULSE_CNTR4", 0x101 },
    { PULSE_CNTR5, "PULSE_CNTR5", 0x101 },
    { PWM0, "PWM0", 0x08E },
    { PWM1, "PWM1", 0x08E },
    { PWM2, "PWM2", 0x08E },
    { PWM3, "PWM3", 0x08E },
    { PWM4, "PWM4", 0x08E },
    { PWM5, "PWM5", 0x08E },
    { QSPI0_CLK, "QSPI0_CLK", 0x08E },
    { QSPI0_CS0, "QSPI0_CS0", 0x0CE },
    { QSPI0_CS1, "QSPI0_CS1", 0x0CE },
    { QSPI0_CS2, "QSPI0_CS2", 0x0CE },
    { QSPI0_CS3, "QSPI0_CS3", 0x0CE },
    { QSPI0_CS4, "QSPI0_CS4", 0x0CE },
    { QSPI0_D0, "QSPI0_D0", 0x18F },
    { QSPI0_D1, "QSPI0_D1", 0x18F },
    { QSPI0_D2, "QSPI0_D2", 0x18F },
    { QSPI0_D3, "QSPI0_D3", 0x18F },
    { QSPI1_CLK, "QSPI1_CLK", 0x08E },
    { QSPI1_CS0, "QSPI1_CS0", 0x0CE },
    { QSPI1_CS1, "QSPI1_CS1", 0x0CE },
    { QSPI1_CS2, "QSPI1_CS2", 0x0CE },
    { QSPI1_CS3, "QSPI1_CS3", 0x0CE },
    { QSPI1_CS4, "QSPI1_CS4", 0x0CE },
    { QSPI1_D0, "QSPI1_D0", 0x18F },
    { QSPI1_D1, "QSPI1_D1", 0x18F },
    { QSPI1_D2, "QSPI1_D2", 0x18F },
    { QSPI1_D3, "QSPI1_D3", 0x18F },
    { SPI2AXI_CK, "SPI2AXI_CK", 0x101 },
    { SPI2AXI_CS, "SPI2AXI_CS", 0x141 },
    { SPI2AXI_DI, "SPI2AXI_DI", 0x101 },
    { SPI2AXI_DO, "SPI2AXI_DO", 0x08E },
    { UART0_RXD, "UART0_RXD", 0x101 },
    { UART0_TXD, "UART0_TXD", 0x08E },
    { UART1_CTS, "UART1_CTS", 0x101 },
    { UART1_RTS, "UART1_RTS", 0x08E },
    { UART1_RXD, "UART1_RXD", 0x101 },
    { UART1_TXD, "UART1_TXD", 0x08E },
    { UART2_CTS, "UART2_CTS", 0x101 },
    { UART2_RTS, "UART2_RTS", 0x08E },
    { UART2_RXD, "UART2_RXD", 0x101 },
    { UART2_TXD, "UART2_TXD", 0x08E },
    { UART3_CTS, "UART3_CTS", 0x101 },
    { UART3_DE, "UART3_DE", 0x08E },
    { UART3_RE, "UART3_RE", 0x08E },
    { UART3_RTS, "UART3_RTS", 0x08E },
    { UART3_RXD, "UART3_RXD", 0x101 },
    { UART3_TXD, "UART3_TXD", 0x08E },
    { UART4_RXD, "UART4_RXD", 0x101 },
    { UART4_TXD, "UART4_TXD", 0x08E },
    { PDM_CLK, "PDM_CLK", 0x08E },
    { VSYNC0, "VSYNC0", 0x08E },
    { VSYNC1, "VSYNC1", 0x08E },
    { CTRL_IN_3D, "CTRL_IN_3D", 0x101 },
    { CTRL_O1_3D, "CTRL_O1_3D", 0x08E },
    { CTRL_O2_3D, "CTRL_O2_3D", 0x08E },
};

static const uint8_t g_pin_func_array[][PIN_FUNC_NUM] = {
    { GPIO0, BOOT0, TEST_PIN0, FUNC_MAX, FUNC_MAX },
    { GPIO1, BOOT1, TEST_PIN1, FUNC_MAX, FUNC_MAX, },
    { GPIO2, JTAG_TCK, PULSE_CNTR0, TEST_PIN2, FUNC_MAX, },
    { GPIO3, JTAG_TDI, PULSE_CNTR1, UART1_TXD, TEST_PIN0, },
    { GPIO4, JTAG_TDO, PULSE_CNTR2, UART1_RXD, TEST_PIN1, },
    { GPIO5, JTAG_TMS, PULSE_CNTR3, UART2_TXD, TEST_PIN2, },
    { GPIO6, JTAG_RST, PULSE_CNTR4, UART2_RXD, TEST_PIN3, },
    { GPIO7, PWM2, IIC4_SCL, TEST_PIN3, DI0, },
    { GPIO8, PWM3, IIC4_SDA, TEST_PIN4, DI1, },
    { GPIO9, PWM4, UART1_TXD, IIC1_SCL, DI2, },
    { GPIO10, CTRL_IN_3D, UART1_RXD, IIC1_SDA, DI3, },
    { GPIO11, CTRL_O1_3D, UART2_TXD, IIC2_SCL, DO0, },
    { GPIO12, CTRL_O2_3D, UART2_RXD, IIC2_SDA, DO1, },
    { GPIO13, M_CLK1, DO2, FUNC_MAX, FUNC_MAX, },
    { GPIO14, OSPI_CS, TEST_PIN5, QSPI0_CS0, DO3, },
    { GPIO15, OSPI_CLK, TEST_PIN6, QSPI0_CLK, CO3, },
    { GPIO16, OSPI_D0, QSPI1_CS4, QSPI0_D0, CO2, },
    { GPIO17, OSPI_D1, QSPI1_CS3, QSPI0_D1, CO1, },
    { GPIO18, OSPI_D2, QSPI1_CS2, QSPI0_D2, CO0, },
    { GPIO19, OSPI_D3, QSPI1_CS1, QSPI0_D3, TEST_PIN4, },
    { GPIO20, OSPI_D4, QSPI1_CS0, PULSE_CNTR0, TEST_PIN5, },
    { GPIO21, OSPI_D5, QSPI1_CLK, PULSE_CNTR1, TEST_PIN6, },
    { GPIO22, OSPI_D6, QSPI1_D0, PULSE_CNTR2, TEST_PIN7, },
    { GPIO23, OSPI_D7, QSPI1_D1, PULSE_CNTR3, TEST_PIN8, },
    { GPIO24, OSPI_DQS, QSPI1_D2, PULSE_CNTR4, TEST_PIN9, },
    { GPIO25, PWM5, QSPI1_D3, PULSE_CNTR5, TEST_PIN10, },
    { GPIO26, MMC1_CLK, TEST_PIN7, PDM_CLK, FUNC_MAX, },
    { GPIO27, MMC1_CMD, PULSE_CNTR5, PDM_IN0, CI0, },
    { GPIO28, MMC1_D0, UART3_TXD, PDM_IN1, CI1, },
    { GPIO29, MMC1_D1, UART3_RXD, CTRL_IN_3D, CI2, },
    { GPIO30, MMC1_D2, UART3_RTS, CTRL_O1_3D, CI3, },
    { GPIO31, MMC1_D3, UART3_CTS, CTRL_O2_3D, TEST_PIN11, },
    { GPIO32, IIC0_SCL, IIS_CLK, UART3_TXD, TEST_PIN12, },
    { GPIO33, IIC0_SDA, IIS_WS, UART3_RXD, TEST_PIN13, },
    { GPIO34, IIC1_SCL, IIS_D_IN0, PDM_IN3, UART3_RTS,  },
    { GPIO35, IIC1_SDA, IIS_D_OUT0, PDM_IN1, UART3_CTS, },
    { GPIO36, IIC3_SCL, IIS_D_IN1, PDM_IN2, UART4_TXD,  },
    { GPIO37, IIC3_SDA, IIS_D_OUT1, PDM_IN0, UART4_RXD, },
    { GPIO38, UART0_TXD, TEST_PIN8, QSPI1_CS0, HSYNC0, },
    { GPIO39, UART0_RXD, TEST_PIN9, QSPI1_CLK, VSYNC0, },
    { GPIO40, UART1_TXD, IIC1_SCL, QSPI1_D0, TEST_PIN18, },
    { GPIO41, UART1_RXD, IIC1_SDA, QSPI1_D1, TEST_PIN19, },
    { GPIO42, UART1_RTS, PWM0, QSPI1_D2, TEST_PIN20, },
    { GPIO43, UART1_CTS, PWM1, QSPI1_D3, TEST_PIN21, },
    { GPIO44, UART2_TXD, IIC3_SCL, TEST_PIN10, SPI2AXI_CK, },
    { GPIO45, UART2_RXD, IIC3_SDA, TEST_PIN11, SPI2AXI_CS, },
    { GPIO46, UART2_RTS, PWM2, IIC4_SCL, TEST_PIN22, },
    { GPIO47, UART2_CTS, PWM3, IIC4_SDA, TEST_PIN23, },
    { GPIO48, UART4_TXD, TEST_PIN12, IIC0_SCL, SPI2AXI_DI, },
    { GPIO49, UART4_RXD, TEST_PIN13, IIC0_SDA, SPI2AXI_DO, },
    { GPIO50, UART3_TXD, IIC2_SCL, QSPI0_CS4, TEST_PIN24, },
    { GPIO51, UART3_RXD, IIC2_SDA, QSPI0_CS3, TEST_PIN25, },
    { GPIO52, UART3_RTS, PWM4, IIC3_SCL, TEST_PIN26, },
    { GPIO53, UART3_CTS, PWM5, IIC3_SDA, FUNC_MAX, },
    { GPIO54, QSPI0_CS0, MMC1_CMD, PWM0, TEST_PIN27, },
    { GPIO55, QSPI0_CLK, MMC1_CLK, PWM1, TEST_PIN28, },
    { GPIO56, QSPI0_D0, MMC1_D0, PWM2, TEST_PIN29, },
    { GPIO57, QSPI0_D1, MMC1_D1, PWM3, TEST_PIN30, },
    { GPIO58, QSPI0_D2, MMC1_D2, PWM4, TEST_PIN31, },
    { GPIO59, QSPI0_D3, MMC1_D3, PWM5, FUNC_MAX, },
    { GPIO60, PWM0, IIC0_SCL, QSPI0_CS2, HSYNC1, },
    { GPIO61, PWM1, IIC0_SDA, QSPI0_CS1, VSYNC1, },
    { GPIO62, M_CLK2, UART3_DE, TEST_PIN14, FUNC_MAX, },
    { GPIO63, M_CLK3, UART3_RE, TEST_PIN15, FUNC_MAX, },
};

#pragma pack ()

static int fpioa_drv_reg_get_or_set(uint32_t pin, uint32_t *value, int set_flage) {
    static volatile uint32_t *reg_base = NULL;

    if (reg_base == NULL) {
        int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (mem_fd < 0) {
            return -1;
        }
        reg_base = (uint32_t *)mmap(NULL, 4 * MAX_PIN_NUM, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, IOMUX_REG_ADD);
        close(mem_fd);
        if (reg_base == NULL) {
            return -2;
        }
    }

    if (set_flage) {
        *(reg_base + pin) = (*(reg_base + pin) & 0x200) | *value;
    } else {
        *value = *(reg_base + pin);
    }

    return 0;
}

int fpioa_drv_reg_set(uint32_t pin, uint32_t value) {
    return fpioa_drv_reg_get_or_set(pin, &value, 1);
}

int fpioa_drv_reg_get(uint32_t pin, uint32_t *value) {
    return fpioa_drv_reg_get_or_set(pin, value, 0);
}

static int fpioa_drv_func_2_pin_io_sel(uint32_t func, uint32_t pin) {
    for (int i = 0; i < PIN_FUNC_NUM; i++) {
        if (func == g_pin_func_array[pin][i]) {
            return i;
        }
    }

    return -1;
}

static int fpioa_drv_get_pin_from_func(uint32_t func) {
    struct st_iomux_reg reg_value;

    for (int i = 0; i < MAX_PIN_NUM; i++) {
        int io_sel = fpioa_drv_func_2_pin_io_sel(func, i);
        if (io_sel < 0) {
            continue;
        }
        if (fpioa_drv_reg_get(i, (uint32_t *)&reg_value)) {
            return -1;
        }
        if (reg_value.u.bit.io_sel == io_sel) {
            return i;
        }
    }

    return -1;
}

static int fpioa_drv_get_pins_from_func(uint32_t func, uint8_t *pins) {
    int count = 0;

    if (func >= GPIO0 && func <= GPIO63) {
        pins[0] = func;
        return 1;
    }

    for (int i = 0; i < MAX_PIN_NUM; i++) {
        int io_sel = fpioa_drv_func_2_pin_io_sel(func, i);
        if (io_sel < 0) {
            continue;
        }
        pins[count] = i;
        count++;
    }

    return count;
}

static int fpioa_drv_get_func_from_pin(uint32_t pin) {
    struct st_iomux_reg reg_value;

    if (fpioa_drv_reg_get(pin, (uint32_t *)&reg_value)) {
        return -1;
    }

    return g_pin_func_array[pin][reg_value.u.bit.io_sel % PIN_FUNC_NUM];
}

static int fpioa_drv_get_func_name_str(uint32_t func, char *str, uint32_t len) {
    if (func >= FUNC_MAX) {
        return 0;
    }
    if (func <= GPIO63) {
        snprintf(str, len - 2, "GPIO%d", func - GPIO0);
    } else if (func < TEST_PIN0) {
        strncpy(str, g_func_describ_array[func - BOOT0].name, len - 2);
    } else if (func < FUNC_MAX) {
        snprintf(str, len - 2, "RESV");
    }
    strncat(str, "/", len - 1);

    return strlen(str);
}

static char *fpioa_drv_get_pin_funcs_str(uint32_t pin, char *str, uint32_t len) {
    uint32_t cur_pos = 0;

    for (int i = 0; i < PIN_FUNC_NUM; i++) {
        cur_pos += fpioa_drv_get_func_name_str(g_pin_func_array[pin][i], str + cur_pos, len - cur_pos);
    }

    return str;
}

static char *fpioa_drv_get_pin_func_str(uint32_t pin, char *str, uint32_t len, int detail_flage) {
    struct st_iomux_reg reg_value;
    int cur_pos = 0;

    if (fpioa_drv_reg_get(pin, (uint32_t *)&reg_value)) {
        return str;
    }

    cur_pos = fpioa_drv_get_func_name_str(g_pin_func_array[pin][reg_value.u.bit.io_sel % PIN_FUNC_NUM], str, len);
    if (cur_pos == 0) {
        return str;
    }

    str[cur_pos - 1] = 0;

    if (detail_flage) {
        str[cur_pos - 1] = ','; // gpio0,ie:,oe:,
        snprintf(str + cur_pos, len - cur_pos,
            "ie:%d,oe:%d,pd:%d,pu:%d,msc:%s,ds:%d,st:%d,sl:%d,di:%d",
            reg_value.u.bit.ie, reg_value.u.bit.oe, reg_value.u.bit.pd, reg_value.u.bit.pu,
            ((reg_value.u.bit.msc) ? "1-1.8v" : "0-3.3v"), reg_value.u.bit.ds, reg_value.u.bit.st,
            reg_value.u.bit.sl, reg_value.u.bit.di);
    }

    return str;
}

static uint32_t fpioa_drv_extract_cfg(mp_arg_val_t *args) {
    enum { ARG_pin, ARG_func, ARG_ie, ARG_oe, ARG_pu, ARG_pd, ARG_ds, ARG_st, ARG_sl };
    int pin, func, sel, tmp;
    struct st_iomux_reg cfg;

    pin = args[ARG_pin].u_int;
    func = args[ARG_func].u_int;
    if (pin < 0 || pin >= MAX_PIN_NUM) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("pin number is invalid"));
    }
    if (func < 0 || func >= FUNC_MAX) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("function number is invalid"));
    }
    sel = fpioa_drv_func_2_pin_io_sel(func, pin);
    if (sel == -1) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("pin not have this function"));
    }
    if (func <= GPIO63) {
        cfg.u.value = 0x18F;
    } else if (func < TEST_PIN0) {
        cfg.u.value = g_func_describ_array[func - BOOT0].default_cfg;
    } else if (func < FUNC_MAX) {
        cfg.u.value = 0;
    }
    cfg.u.bit.io_sel = sel;
    tmp = args[ARG_ie].u_int;
    if (tmp != -1) {
        cfg.u.bit.ie = tmp > 0 ? 1 : 0;
    }
    tmp = args[ARG_oe].u_int;
    if (tmp != -1) {
        cfg.u.bit.oe = tmp > 0 ? 1 : 0;
    }
    tmp = args[ARG_pu].u_int;
    if (tmp != -1) {
        cfg.u.bit.pu = tmp > 0 ? 1 : 0;
    }
    tmp = args[ARG_pd].u_int;
    if (tmp != -1) {
        cfg.u.bit.pd = tmp > 0 ? 1 : 0;
    }
    tmp = args[ARG_st].u_int;
    if (tmp != -1) {
        cfg.u.bit.st = tmp > 0 ? 1 : 0;
    }
    tmp = args[ARG_sl].u_int;
    if (tmp != -1) {
        cfg.u.bit.sl = tmp > 0 ? 1 : 0;
    }
    tmp = args[ARG_ds].u_int;
    if (tmp != -1) {
        cfg.u.bit.ds = tmp < 0 ? 0 : tmp > 0xF ? 0xF : tmp;
    }

    return cfg.u.value;
}

STATIC void machine_fpioa_help_print_pin_func(int pin_num, int detail_flag) {
    char str_tmp[TEMP_STR_LEN];
    char str_tmp1[TEMP_STR_LEN];

    if (pin_num == -1) {
        for (int i = 0; i < MAX_PIN_NUM; i++) {
            machine_fpioa_help_print_pin_func(i, detail_flag);
        }
        return;
    }
    memset(str_tmp, 0, sizeof(str_tmp));
    memset(str_tmp1, 0, sizeof(str_tmp1));
    fpioa_drv_get_pin_funcs_str(pin_num, str_tmp, sizeof(str_tmp));
    fpioa_drv_get_pin_func_str(pin_num, str_tmp1, sizeof(str_tmp1), detail_flag);

    if (detail_flag) {
        mp_printf(&mp_plat_print, "|%-17s|%-60s|\r\n", "current config", str_tmp1);
        mp_printf(&mp_plat_print, "|%-17s|%-60s|\r\n", "can be function", str_tmp);
    } else {
        mp_printf(&mp_plat_print, "| %-2d   | %-10s | %-56s|\r\n", pin_num, str_tmp1, str_tmp);
    }
}

STATIC mp_obj_t machine_fpioa_set_function(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_pin, ARG_func, ARG_ie, ARG_oe, ARG_pu, ARG_pd, ARG_ds, ARG_st, ARG_sl };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_func, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_ie, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
        { MP_QSTR_oe, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
        { MP_QSTR_pu, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
        { MP_QSTR_pd, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
        { MP_QSTR_ds, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
        { MP_QSTR_st, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
        { MP_QSTR_sl, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = -1 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    int pin = args[ARG_pin].u_int;
    int func = args[ARG_func].u_int;
    uint32_t cfg = fpioa_drv_extract_cfg(args);
    uint8_t pins[FUNC_ALT_PIN_NUM];
    int pin_count = fpioa_drv_get_pins_from_func(func, pins);
    for (int i = 0; i < pin_count; i++) {
        if (pins[i] == pin || fpioa_drv_get_func_from_pin(pins[i]) != func) {
            continue;
        }
        fpioa_drv_reg_set(pins[i], 0);
    }

    fpioa_drv_reg_set(pin, cfg);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_fpioa_set_function_obj, 3, machine_fpioa_set_function);

STATIC mp_obj_t machine_fpioa_get_pin_num(mp_obj_t self, mp_obj_t obj) {
    int func, pin;

    func = mp_obj_get_int(obj);
    if (func < 0 || func >= FUNC_MAX) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("function number is invalid"));
    }

    pin = fpioa_drv_get_pin_from_func(func);

    return pin < 0 ? mp_const_none : MP_OBJ_NEW_SMALL_INT(pin);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_fpioa_get_pin_num_obj, machine_fpioa_get_pin_num);

STATIC mp_obj_t machine_fpioa_get_pin_func(mp_obj_t self, mp_obj_t obj) {
    int func, pin;

    pin = mp_obj_get_int(obj);
    if (pin < 0 || pin >= MAX_PIN_NUM) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("pin number is invalid"));
    }

    func = fpioa_drv_get_func_from_pin(pin);

    return func < 0 ? mp_const_none : MP_OBJ_NEW_SMALL_INT(func);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_fpioa_get_pin_func_obj, machine_fpioa_get_pin_func);

STATIC mp_obj_t machine_fpioa_help(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_num, ARG_func };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_num, MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_func, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    if (args[ARG_func].u_bool == false) { // pin mode
        int pin = args[ARG_num].u_int;
        if (pin != -1) {
            if ((pin >= MAX_PIN_NUM) || (pin < 0)) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("pin number is invalid"));
            }
            mp_printf(&mp_plat_print, "|%-17s|%-60d|\r\n", "pin num ", pin);
            machine_fpioa_help_print_pin_func(pin, 1);
        } else {
            mp_printf(&mp_plat_print, "| pin  | cur func   |                can be func                              |\r\n");
            mp_printf(&mp_plat_print, "| ---- |------------|---------------------------------------------------------|\r\n");
            machine_fpioa_help_print_pin_func(-1, 0);
        }
    } else { // func mode
        int func = args[ARG_num].u_int;
        if (func < 0 || func >= FUNC_MAX) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("function number is invalid"));
        }
        uint8_t pins[FUNC_ALT_PIN_NUM];
        int count;
        char str_tmp[TEMP_STR_LEN];
        int len;
        count = fpioa_drv_get_pins_from_func(func, pins);
        len = fpioa_drv_get_func_name_str(func, str_tmp, TEMP_STR_LEN);
        str_tmp[len - 1] = 0;
        mp_printf(&mp_plat_print, "%s function can be set to ", str_tmp);
        for (int i = 0; i < count; i++) {
            mp_printf(&mp_plat_print, "PIN%d%s", pins[i], i + 1 == count ? "" : ", ");
        }
        mp_printf(&mp_plat_print, "\r\n");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_fpioa_help_obj, 1, machine_fpioa_help);

STATIC void machine_fpioa_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_fpioa_help(1, NULL, (mp_map_t *)&mp_const_empty_map);
}

STATIC mp_obj_t machine_fpioa_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    return (mp_obj_t)&machine_fpioa_obj;
}

STATIC const mp_rom_map_elem_t machine_fpioa_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_function), MP_ROM_PTR(&machine_fpioa_set_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pin_num), MP_ROM_PTR(&machine_fpioa_get_pin_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pin_func), MP_ROM_PTR(&machine_fpioa_get_pin_func_obj) },
    { MP_ROM_QSTR(MP_QSTR_help), MP_ROM_PTR(&machine_fpioa_help_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO0), MP_ROM_INT(GPIO0) },
    { MP_ROM_QSTR(MP_QSTR_GPIO1), MP_ROM_INT(GPIO1) },
    { MP_ROM_QSTR(MP_QSTR_GPIO2), MP_ROM_INT(GPIO2) },
    { MP_ROM_QSTR(MP_QSTR_GPIO3), MP_ROM_INT(GPIO3) },
    { MP_ROM_QSTR(MP_QSTR_GPIO4), MP_ROM_INT(GPIO4) },
    { MP_ROM_QSTR(MP_QSTR_GPIO5), MP_ROM_INT(GPIO5) },
    { MP_ROM_QSTR(MP_QSTR_GPIO6), MP_ROM_INT(GPIO6) },
    { MP_ROM_QSTR(MP_QSTR_GPIO7), MP_ROM_INT(GPIO7) },
    { MP_ROM_QSTR(MP_QSTR_GPIO8), MP_ROM_INT(GPIO8) },
    { MP_ROM_QSTR(MP_QSTR_GPIO9), MP_ROM_INT(GPIO9) },
    { MP_ROM_QSTR(MP_QSTR_GPIO10), MP_ROM_INT(GPIO10) },
    { MP_ROM_QSTR(MP_QSTR_GPIO11), MP_ROM_INT(GPIO11) },
    { MP_ROM_QSTR(MP_QSTR_GPIO12), MP_ROM_INT(GPIO12) },
    { MP_ROM_QSTR(MP_QSTR_GPIO13), MP_ROM_INT(GPIO13) },
    { MP_ROM_QSTR(MP_QSTR_GPIO14), MP_ROM_INT(GPIO14) },
    { MP_ROM_QSTR(MP_QSTR_GPIO15), MP_ROM_INT(GPIO15) },
    { MP_ROM_QSTR(MP_QSTR_GPIO16), MP_ROM_INT(GPIO16) },
    { MP_ROM_QSTR(MP_QSTR_GPIO17), MP_ROM_INT(GPIO17) },
    { MP_ROM_QSTR(MP_QSTR_GPIO18), MP_ROM_INT(GPIO18) },
    { MP_ROM_QSTR(MP_QSTR_GPIO19), MP_ROM_INT(GPIO19) },
    { MP_ROM_QSTR(MP_QSTR_GPIO20), MP_ROM_INT(GPIO20) },
    { MP_ROM_QSTR(MP_QSTR_GPIO21), MP_ROM_INT(GPIO21) },
    { MP_ROM_QSTR(MP_QSTR_GPIO22), MP_ROM_INT(GPIO22) },
    { MP_ROM_QSTR(MP_QSTR_GPIO23), MP_ROM_INT(GPIO23) },
    { MP_ROM_QSTR(MP_QSTR_GPIO24), MP_ROM_INT(GPIO24) },
    { MP_ROM_QSTR(MP_QSTR_GPIO25), MP_ROM_INT(GPIO25) },
    { MP_ROM_QSTR(MP_QSTR_GPIO26), MP_ROM_INT(GPIO26) },
    { MP_ROM_QSTR(MP_QSTR_GPIO27), MP_ROM_INT(GPIO27) },
    { MP_ROM_QSTR(MP_QSTR_GPIO28), MP_ROM_INT(GPIO28) },
    { MP_ROM_QSTR(MP_QSTR_GPIO29), MP_ROM_INT(GPIO29) },
    { MP_ROM_QSTR(MP_QSTR_GPIO30), MP_ROM_INT(GPIO30) },
    { MP_ROM_QSTR(MP_QSTR_GPIO31), MP_ROM_INT(GPIO31) },
    { MP_ROM_QSTR(MP_QSTR_GPIO32), MP_ROM_INT(GPIO32) },
    { MP_ROM_QSTR(MP_QSTR_GPIO33), MP_ROM_INT(GPIO33) },
    { MP_ROM_QSTR(MP_QSTR_GPIO34), MP_ROM_INT(GPIO34) },
    { MP_ROM_QSTR(MP_QSTR_GPIO35), MP_ROM_INT(GPIO35) },
    { MP_ROM_QSTR(MP_QSTR_GPIO36), MP_ROM_INT(GPIO36) },
    { MP_ROM_QSTR(MP_QSTR_GPIO37), MP_ROM_INT(GPIO37) },
    { MP_ROM_QSTR(MP_QSTR_GPIO38), MP_ROM_INT(GPIO38) },
    { MP_ROM_QSTR(MP_QSTR_GPIO39), MP_ROM_INT(GPIO39) },
    { MP_ROM_QSTR(MP_QSTR_GPIO40), MP_ROM_INT(GPIO40) },
    { MP_ROM_QSTR(MP_QSTR_GPIO41), MP_ROM_INT(GPIO41) },
    { MP_ROM_QSTR(MP_QSTR_GPIO42), MP_ROM_INT(GPIO42) },
    { MP_ROM_QSTR(MP_QSTR_GPIO43), MP_ROM_INT(GPIO43) },
    { MP_ROM_QSTR(MP_QSTR_GPIO44), MP_ROM_INT(GPIO44) },
    { MP_ROM_QSTR(MP_QSTR_GPIO45), MP_ROM_INT(GPIO45) },
    { MP_ROM_QSTR(MP_QSTR_GPIO46), MP_ROM_INT(GPIO46) },
    { MP_ROM_QSTR(MP_QSTR_GPIO47), MP_ROM_INT(GPIO47) },
    { MP_ROM_QSTR(MP_QSTR_GPIO48), MP_ROM_INT(GPIO48) },
    { MP_ROM_QSTR(MP_QSTR_GPIO49), MP_ROM_INT(GPIO49) },
    { MP_ROM_QSTR(MP_QSTR_GPIO50), MP_ROM_INT(GPIO50) },
    { MP_ROM_QSTR(MP_QSTR_GPIO51), MP_ROM_INT(GPIO51) },
    { MP_ROM_QSTR(MP_QSTR_GPIO52), MP_ROM_INT(GPIO52) },
    { MP_ROM_QSTR(MP_QSTR_GPIO53), MP_ROM_INT(GPIO53) },
    { MP_ROM_QSTR(MP_QSTR_GPIO54), MP_ROM_INT(GPIO54) },
    { MP_ROM_QSTR(MP_QSTR_GPIO55), MP_ROM_INT(GPIO55) },
    { MP_ROM_QSTR(MP_QSTR_GPIO56), MP_ROM_INT(GPIO56) },
    { MP_ROM_QSTR(MP_QSTR_GPIO57), MP_ROM_INT(GPIO57) },
    { MP_ROM_QSTR(MP_QSTR_GPIO58), MP_ROM_INT(GPIO58) },
    { MP_ROM_QSTR(MP_QSTR_GPIO59), MP_ROM_INT(GPIO59) },
    { MP_ROM_QSTR(MP_QSTR_GPIO60), MP_ROM_INT(GPIO60) },
    { MP_ROM_QSTR(MP_QSTR_GPIO61), MP_ROM_INT(GPIO61) },
    { MP_ROM_QSTR(MP_QSTR_GPIO62), MP_ROM_INT(GPIO62) },
    { MP_ROM_QSTR(MP_QSTR_GPIO63), MP_ROM_INT(GPIO63) },
    { MP_ROM_QSTR(MP_QSTR_BOOT0), MP_ROM_INT(BOOT0) },
    { MP_ROM_QSTR(MP_QSTR_BOOT1), MP_ROM_INT(BOOT1) },
    { MP_ROM_QSTR(MP_QSTR_HSYNC0), MP_ROM_INT(HSYNC0) },
    { MP_ROM_QSTR(MP_QSTR_HSYNC1), MP_ROM_INT(HSYNC1) },
    { MP_ROM_QSTR(MP_QSTR_IIC0_SCL), MP_ROM_INT(IIC0_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC0_SDA), MP_ROM_INT(IIC0_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC1_SCL), MP_ROM_INT(IIC1_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC1_SDA), MP_ROM_INT(IIC1_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC2_SCL), MP_ROM_INT(IIC2_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC2_SDA), MP_ROM_INT(IIC2_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC3_SCL), MP_ROM_INT(IIC3_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC3_SDA), MP_ROM_INT(IIC3_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC4_SCL), MP_ROM_INT(IIC4_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC4_SDA), MP_ROM_INT(IIC4_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIS_CLK), MP_ROM_INT(IIS_CLK) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_IN0), MP_ROM_INT(IIS_D_IN0) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_IN1), MP_ROM_INT(IIS_D_IN1) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_OUT0), MP_ROM_INT(IIS_D_OUT0) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_OUT1), MP_ROM_INT(IIS_D_OUT1) },
    { MP_ROM_QSTR(MP_QSTR_IIS_WS), MP_ROM_INT(IIS_WS) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_RST), MP_ROM_INT(JTAG_RST) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TCK), MP_ROM_INT(JTAG_TCK) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TDI), MP_ROM_INT(JTAG_TDI) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TDO), MP_ROM_INT(JTAG_TDO) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TMS), MP_ROM_INT(JTAG_TMS) },
    { MP_ROM_QSTR(MP_QSTR_M_CLK1), MP_ROM_INT(M_CLK1) },
    { MP_ROM_QSTR(MP_QSTR_M_CLK2), MP_ROM_INT(M_CLK2) },
    { MP_ROM_QSTR(MP_QSTR_M_CLK3), MP_ROM_INT(M_CLK3) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_CLK), MP_ROM_INT(MMC1_CLK) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_CMD), MP_ROM_INT(MMC1_CMD) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D0), MP_ROM_INT(MMC1_D0) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D1), MP_ROM_INT(MMC1_D1) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D2), MP_ROM_INT(MMC1_D2) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D3), MP_ROM_INT(MMC1_D3) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_CLK), MP_ROM_INT(OSPI_CLK) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_CS), MP_ROM_INT(OSPI_CS) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D0), MP_ROM_INT(OSPI_D0) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D1), MP_ROM_INT(OSPI_D1) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D2), MP_ROM_INT(OSPI_D2) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D3), MP_ROM_INT(OSPI_D3) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D4), MP_ROM_INT(OSPI_D4) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D5), MP_ROM_INT(OSPI_D5) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D6), MP_ROM_INT(OSPI_D6) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D7), MP_ROM_INT(OSPI_D7) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_DQS), MP_ROM_INT(OSPI_DQS) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN0), MP_ROM_INT(PDM_IN0) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN1), MP_ROM_INT(PDM_IN1) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN2), MP_ROM_INT(PDM_IN2) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN3), MP_ROM_INT(PDM_IN3) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR0), MP_ROM_INT(PULSE_CNTR0) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR1), MP_ROM_INT(PULSE_CNTR1) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR2), MP_ROM_INT(PULSE_CNTR2) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR3), MP_ROM_INT(PULSE_CNTR3) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR4), MP_ROM_INT(PULSE_CNTR4) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR5), MP_ROM_INT(PULSE_CNTR5) },
    { MP_ROM_QSTR(MP_QSTR_PWM0), MP_ROM_INT(PWM0) },
    { MP_ROM_QSTR(MP_QSTR_PWM1), MP_ROM_INT(PWM1) },
    { MP_ROM_QSTR(MP_QSTR_PWM2), MP_ROM_INT(PWM2) },
    { MP_ROM_QSTR(MP_QSTR_PWM3), MP_ROM_INT(PWM3) },
    { MP_ROM_QSTR(MP_QSTR_PWM4), MP_ROM_INT(PWM4) },
    { MP_ROM_QSTR(MP_QSTR_PWM5), MP_ROM_INT(PWM5) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CLK), MP_ROM_INT(QSPI0_CLK) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS0), MP_ROM_INT(QSPI0_CS0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS1), MP_ROM_INT(QSPI0_CS1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS2), MP_ROM_INT(QSPI0_CS2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS3), MP_ROM_INT(QSPI0_CS3) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS4), MP_ROM_INT(QSPI0_CS4) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D0), MP_ROM_INT(QSPI0_D0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D1), MP_ROM_INT(QSPI0_D1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D2), MP_ROM_INT(QSPI0_D2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D3), MP_ROM_INT(QSPI0_D3) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CLK), MP_ROM_INT(QSPI1_CLK) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS0), MP_ROM_INT(QSPI1_CS0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS1), MP_ROM_INT(QSPI1_CS1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS2), MP_ROM_INT(QSPI1_CS2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS3), MP_ROM_INT(QSPI1_CS3) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS4), MP_ROM_INT(QSPI1_CS4) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D0), MP_ROM_INT(QSPI1_D0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D1), MP_ROM_INT(QSPI1_D1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D2), MP_ROM_INT(QSPI1_D2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D3), MP_ROM_INT(QSPI1_D3) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_CK), MP_ROM_INT(SPI2AXI_CK) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_CS), MP_ROM_INT(SPI2AXI_CS) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_DI), MP_ROM_INT(SPI2AXI_DI) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_DO), MP_ROM_INT(SPI2AXI_DO) },
    { MP_ROM_QSTR(MP_QSTR_UART0_RXD), MP_ROM_INT(UART0_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART0_TXD), MP_ROM_INT(UART0_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART1_CTS), MP_ROM_INT(UART1_CTS) },
    { MP_ROM_QSTR(MP_QSTR_UART1_RTS), MP_ROM_INT(UART1_RTS) },
    { MP_ROM_QSTR(MP_QSTR_UART1_RXD), MP_ROM_INT(UART1_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART1_TXD), MP_ROM_INT(UART1_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART2_CTS), MP_ROM_INT(UART2_CTS) },
    { MP_ROM_QSTR(MP_QSTR_UART2_RTS), MP_ROM_INT(UART2_RTS) },
    { MP_ROM_QSTR(MP_QSTR_UART2_RXD), MP_ROM_INT(UART2_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART2_TXD), MP_ROM_INT(UART2_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART3_CTS), MP_ROM_INT(UART3_CTS) },
    { MP_ROM_QSTR(MP_QSTR_UART3_DE), MP_ROM_INT(UART3_DE) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RE), MP_ROM_INT(UART3_RE) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RTS), MP_ROM_INT(UART3_RTS) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RXD), MP_ROM_INT(UART3_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART3_TXD), MP_ROM_INT(UART3_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART4_RXD), MP_ROM_INT(UART4_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART4_TXD), MP_ROM_INT(UART4_TXD) },
    { MP_ROM_QSTR(MP_QSTR_PDM_CLK), MP_ROM_INT(PDM_CLK) },
    { MP_ROM_QSTR(MP_QSTR_VSYNC0), MP_ROM_INT(VSYNC0) },
    { MP_ROM_QSTR(MP_QSTR_VSYNC1), MP_ROM_INT(VSYNC1) },
    { MP_ROM_QSTR(MP_QSTR_CTRL_IN_3D), MP_ROM_INT(CTRL_IN_3D) },
    { MP_ROM_QSTR(MP_QSTR_CTRL_O1_3D), MP_ROM_INT(CTRL_O1_3D) },
    { MP_ROM_QSTR(MP_QSTR_CTRL_O2_3D), MP_ROM_INT(CTRL_O2_3D) },
};
STATIC MP_DEFINE_CONST_DICT(machine_fpioa_locals_dict, machine_fpioa_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_fpioa_type,
    MP_QSTR_FPIOA,
    MP_TYPE_FLAG_NONE,
    make_new, machine_fpioa_make_new,
    print, machine_fpioa_print,
    locals_dict, &machine_fpioa_locals_dict);
