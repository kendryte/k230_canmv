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
#include "machine_iomux_fpioa.h"
#include "py/runtime.h"
#include "py/obj.h"
#include <sys/mman.h>

#define MAX_PIN_NUM                64
#define TEMP_STR_LEN 128
#define IOMUX_REG_ADD 0X91105000
enum {
    ARG_pin,
    ARG_func,
    ARG_set_st,
    ARG_set_ds,
    ARG_set_pd,
    ARG_set_pu,
    ARG_set_oe,
    ARG_set_ie,
    ARG_set_msc,
    ARG_set_sl,    
    ARG_set_di,
};

#pragma pack (1)

static const struct  st_func_describe g_func_describ_array[]={
    {BOOT0,   "BOOT0" ,"" },
    {BOOT1,   "BOOT1" ,"" },
    {CI0,   "CI0" ,"" },
    {CI1,   "CI1" ,"" },
    {CI2,   "CI2" ,"" },
    {CI3,   "CI3" ,"" },
    {CO0,   "CO0" ,"" },
    {CO1,   "CO1" ,"" },
    {CO2,   "CO2" ,"" },
    {CO3,   "CO3" ,"" },
    {DI0,   "DI0" ,"" },
    {DI1,   "DI1" ,"" },
    {DI2,   "DI2" ,"" },
    {DI3,   "DI3" ,"" },
    {DO0,   "DO0" ,"" },
    {DO1,   "DO1" ,"" },
    {DO2,   "DO2" ,"" },
    {DO3,   "DO3" ,"" },
    {HSYNC0,   "HSYNC0" ,"" },
    {HSYNC1,   "HSYNC1" ,"" },
    {IIC0_SCL,   "IIC0_SCL" ,"" },
    {IIC0_SDA,   "IIC0_SDA" ,"" },
    {IIC1_SCL,   "IIC1_SCL" ,"" },
    {IIC1_SDA,   "IIC1_SDA" ,"" },
    {IIC2_SCL,   "IIC2_SCL" ,"" },
    {IIC2_SDA,   "IIC2_SDA" ,"" },
    {IIC3_SCL,   "IIC3_SCL" ,"" },
    {IIC3_SDA,   "IIC3_SDA" ,"" },
    {IIC4_SCL,   "IIC4_SCL" ,"" },
    {IIC4_SDA,   "IIC4_SDA" ,"" },
    {IIS_CLK,   "IIS_CLK" ,"" },
    {IIS_D_IN0,   "IIS_D_IN0" ,"" },
    {IIS_D_IN1,   "IIS_D_IN1" ,"" },
    {IIS_D_OUT0,   "IIS_D_OUT0" ,"" },
    {IIS_D_OUT1,   "IIS_D_OUT1" ,"" },
    {IIS_WS,   "IIS_WS" ,"" },
    {JTAG_RST,   "JTAG_RST" ,"" },
    {JTAG_TCK,   "JTAG_TCK" ,"" },
    {JTAG_TDI,   "JTAG_TDI" ,"" },
    {JTAG_TDO,   "JTAG_TDO" ,"" },
    {JTAG_TMS,   "JTAG_TMS" ,"" },
    {M_CLK1,   "M_CLK1" ,"" },
    {M_CLK2,   "M_CLK2" ,"" },
    {M_CLK3,   "M_CLK3" ,"" },
    {MMC1_CLK,   "MMC1_CLK" ,"" },
    {MMC1_CMD,   "MMC1_CMD" ,"" },
    {MMC1_D0,   "MMC1_D0" ,"" },
    {MMC1_D1,   "MMC1_D1" ,"" },
    {MMC1_D2,   "MMC1_D2" ,"" },
    {MMC1_D3,   "MMC1_D3" ,"" },
    {OSPI_CLK,   "OSPI_CLK" ,"" },
    {OSPI_CS,   "OSPI_CS" ,"" },
    {OSPI_D0,   "OSPI_D0" ,"" },
    {OSPI_D1,   "OSPI_D1" ,"" },
    {OSPI_D2,   "OSPI_D2" ,"" },
    {OSPI_D3,   "OSPI_D3" ,"" },
    {OSPI_D4,   "OSPI_D4" ,"" },
    {OSPI_D5,   "OSPI_D5" ,"" },
    {OSPI_D6,   "OSPI_D6" ,"" },
    {OSPI_D7,   "OSPI_D7" ,"" },
    {OSPI_DQS,   "OSPI_DQS" ,"" },
    {PDM_IN0,   "PDM_IN0" ,"" },
    {PDM_IN1,   "PDM_IN1" ,"" },
    {PDM_IN2,   "PDM_IN2" ,"" },
    {PDM_IN3,   "PDM_IN3" ,"" },
    {PULSE_CNTR0,   "PULSE_CNTR0" ,"" },
    {PULSE_CNTR1,   "PULSE_CNTR1" ,"" },
    {PULSE_CNTR2,   "PULSE_CNTR2" ,"" },
    {PULSE_CNTR3,   "PULSE_CNTR3" ,"" },
    {PULSE_CNTR4,   "PULSE_CNTR4" ,"" },
    {PULSE_CNTR5,   "PULSE_CNTR5" ,"" },
    {PWM0,   "PWM0" ,"" },
    {PWM1,   "PWM1" ,"" },
    {PWM2,   "PWM2" ,"" },
    {PWM3,   "PWM3" ,"" },
    {PWM4,   "PWM4" ,"" },
    {PWM5,   "PWM5" ,"" },
    {QSPI0_CLK,   "QSPI0_CLK" ,"" },
    {QSPI0_CS0,   "QSPI0_CS0" ,"" },
    {QSPI0_CS1,   "QSPI0_CS1" ,"" },
    {QSPI0_CS2,   "QSPI0_CS2" ,"" },
    {QSPI0_CS3,   "QSPI0_CS3" ,"" },
    {QSPI0_CS4,   "QSPI0_CS4" ,"" },
    {QSPI0_D0,   "QSPI0_D0" ,"" },
    {QSPI0_D1,   "QSPI0_D1" ,"" },
    {QSPI0_D2,   "QSPI0_D2" ,"" },
    {QSPI0_D3,   "QSPI0_D3" ,"" },
    {QSPI1_CLK,   "QSPI1_CLK" ,"" },
    {QSPI1_CS0,   "QSPI1_CS0" ,"" },
    {QSPI1_CS1,   "QSPI1_CS1" ,"" },
    {QSPI1_CS2,   "QSPI1_CS2" ,"" },
    {QSPI1_CS3,   "QSPI1_CS3" ,"" },
    {QSPI1_CS4,   "QSPI1_CS4" ,"" },
    {QSPI1_D0,   "QSPI1_D0" ,"" },
    {QSPI1_D1,   "QSPI1_D1" ,"" },
    {QSPI1_D2,   "QSPI1_D2" ,"" },
    {QSPI1_D3,   "QSPI1_D3" ,"" },
    {SPI2AXI_CK,   "SPI2AXI_CK" ,"" },
    {SPI2AXI_CS,   "SPI2AXI_CS" ,"" },
    {SPI2AXI_DI,   "SPI2AXI_DI" ,"" },
    {SPI2AXI_DO,   "SPI2AXI_DO" ,"" },
    {UART0_RXD,   "UART0_RXD" ,"" },
    {UART0_TXD,   "UART0_TXD" ,"" },
    {UART1_CTS,   "UART1_CTS" ,"" },
    {UART1_RTS,   "UART1_RTS" ,"" },
    {UART1_RXD,   "UART1_RXD" ,"" },
    {UART1_TXD,   "UART1_TXD" ,"" },
    {UART2_CTS,   "UART2_CTS" ,"" },
    {UART2_RTS,   "UART2_RTS" ,"" },
    {UART2_RXD,   "UART2_RXD" ,"" },
    {UART2_TXD,   "UART2_TXD" ,"" },
    {UART3_CTS,   "UART3_CTS" ,"" },
    {UART3_DE,   "UART3_DE" ,"" },
    {UART3_RE,   "UART3_RE" ,"" },
    {UART3_RTS,   "UART3_RTS" ,"" },
    {UART3_RXD,   "UART3_RXD" ,"" },
    {UART3_TXD,   "UART3_TXD" ,"" },
    {UART4_RXD,   "UART4_RXD" ,"" },
    {UART4_TXD,   "UART4_TXD" ,"" },
    {PDM_CLK,   "PDM_CLK" ,"" },
    {VSYNC0,   "VSYNC0" ,"" },
    {VSYNC1,   "VSYNC1" ,"" },
    {CTRL_IN_3D,   "CTRL_IN_3D" ,"" },
    {CTRL_O1_3D,   "CTRL_O1_3D" ,"" },
    {CTRL_O2_3D,   "CTRL_O2_3D" ,"" },
};


//pin_func_str;
static const struct st_pin_func g_pin_func_array[]=
{
    { GPIO0, BOOT0, TEST_PIN0, FUNC_MAX, FUNC_MAX },
    { GPIO1, BOOT1, TEST_PIN1,FUNC_MAX,FUNC_MAX, },
    { GPIO2, JTAG_TCK, PULSE_CNTR0, TEST_PIN2, FUNC_MAX,},
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
    { GPIO13, M_CLK1, DO2, FUNC_MAX,FUNC_MAX,},
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
    { GPIO26, MMC1_CLK, TEST_PIN7,PDM_CLK, FUNC_MAX,},
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
    { GPIO53, UART3_CTS, PWM5, IIC3_SDA,FUNC_MAX, },
    { GPIO54, QSPI0_CS0, MMC1_CMD, PWM0, TEST_PIN27, },
    { GPIO55, QSPI0_CLK, MMC1_CLK, PWM1, TEST_PIN28, },
    { GPIO56, QSPI0_D0, MMC1_D0, PWM2, TEST_PIN29, },
    { GPIO57, QSPI0_D1, MMC1_D1, PWM3, TEST_PIN30, },
    { GPIO58, QSPI0_D2, MMC1_D2, PWM4, TEST_PIN31, },
    { GPIO59, QSPI0_D3, MMC1_D3, PWM5, FUNC_MAX,},
    { GPIO60, PWM0, IIC0_SCL, QSPI0_CS2, HSYNC1, },
    { GPIO61, PWM1, IIC0_SDA, QSPI0_CS1, VSYNC1, },
    { GPIO62, M_CLK2, UART3_DE, TEST_PIN14,FUNC_MAX, },
    { GPIO63, M_CLK3, UART3_RE, TEST_PIN15 ,FUNC_MAX, },
};

#pragma pack ()
// 0 ---ok other ---failed
static int  fpioa_drv_reg_get_or_set(uint32_t pin, uint32_t *value, int set_flage)
{
    int ret = -1;
    uint32_t * regadd = NULL;
    int mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
    if(mem_fd < 0)
        return -2;

    regadd = (uint32_t *)mmap(NULL, 4*MAX_PIN_NUM, \
                                    PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, IOMUX_REG_ADD);
    if(regadd != MAP_FAILED ){
        if(set_flage){
            *(regadd + pin) = *value;
            //printf("write pin %d add =%p valu=%x %p\n", pin, regadd, *value, regadd + pin );
        }else {
            *value =  *(regadd + pin);
            //printf("read pin %d add =%p valu=%x %p\n", pin, regadd, *value, regadd + pin );
        }
        munmap(regadd, 4*MAX_PIN_NUM);
        ret = 0;
    }
    close(mem_fd);
    return ret;
}
//0---ok 
int fpioa_drv_reg_set(uint32_t pin, uint32_t value)
{
    return fpioa_drv_reg_get_or_set(pin, &value, 1);
}
//0---ok 
int fpioa_drv_reg_get(uint32_t pin, uint32_t *value)
{
    return fpioa_drv_reg_get_or_set(pin, value, 0);
}
// -1 ---error 
int fpioa_drv_func_2_pin_io_sel(uint32_t func, uint32_t pin)
{
    int io_sel = -1;

     if(func == g_pin_func_array[pin].func0)
        io_sel = 0;
    else if (func == g_pin_func_array[pin].func1)
        io_sel = 1;
    else if (func == g_pin_func_array[pin].func2)
        io_sel = 2;
    else if (func == g_pin_func_array[pin].func3)
        io_sel = 3;
    else if (func == g_pin_func_array[pin].func4)
        io_sel = 4;

    return io_sel;
}

static int  fpioa_drv_get_pin_from_func(uint32_t func,uint8_t pins[])
{
    int count  = 0;
    int i = 0;
    int io_sel = 0;
    struct st_iomux_reg reg_value;

    for(i = 0; i < MAX_PIN_NUM; i++){
        io_sel = fpioa_drv_func_2_pin_io_sel(func, i);
        if(io_sel < 0)
            continue;        
        if(fpioa_drv_reg_get(i, (uint32_t*)&reg_value)){
            count = -1;
            break;
        }       
        if(reg_value.u.bit.io_sel == io_sel){
            pins[count++] = i;
         }
    }

    return count;  
}

static int  fpioa_get_func_name_str(uint32_t func ,char *str, uint32_t len)
{
    if (func >= FUNC_MAX){
        return 0;
    }
    if( func <= GPIO63 ){
        snprintf(str, len-2, "GPIO%d", func-GPIO0);        
    }else if (func < TEST_PIN0 ){   
        strncpy(str, g_func_describ_array[func - BOOT0].name, len-2); 
    }else if (func < FUNC_MAX){
        snprintf(str, len-2, "TEST_PIN%d", func-TEST_PIN0);
    }
    strcat(str,"/");
    return strlen(str);
}

static char * fpioa_get_pin_funcs_str(uint32_t pin ,char *str, uint32_t len)
{
    uint32_t cur_pos=0;
    cur_pos += fpioa_get_func_name_str(g_pin_func_array[pin].func0, str + cur_pos, len - cur_pos);
    cur_pos += fpioa_get_func_name_str(g_pin_func_array[pin].func1, str + cur_pos, len - cur_pos);
    cur_pos += fpioa_get_func_name_str(g_pin_func_array[pin].func2, str + cur_pos, len - cur_pos);
    cur_pos += fpioa_get_func_name_str(g_pin_func_array[pin].func3, str + cur_pos, len - cur_pos);
    cur_pos += fpioa_get_func_name_str(g_pin_func_array[pin].func4, str + cur_pos, len - cur_pos);
    return str;
}
static char * fpioa_get_pin_cur_func_str(uint32_t pin ,char *str, uint32_t len, int detail_flage)
{
    struct st_iomux_reg reg_value;
    const uint8_t *pfunc = &g_pin_func_array[pin].func0;
    int cur_pos = 0;

    if(fpioa_drv_reg_get(pin, (uint32_t*)&reg_value))
        return str;

    cur_pos = fpioa_get_func_name_str(*(pfunc + reg_value.u.bit.io_sel), str, len);
    if(cur_pos == 0)
        return str;
    
    str[cur_pos - 1]=0;

    if(detail_flage){
        str[cur_pos - 1]= ',';//gpio0,ie:,oe:,
        snprintf(str+cur_pos, len-cur_pos,\
                "ie:%d,oe:%d,pd:%d,pu:%d,msc:%s,ds:%d,st:%d,sl:%d,di:%d",\
                reg_value.u.bit.ie, reg_value.u.bit.oe,reg_value.u.bit.pd, reg_value.u.bit.pu,\
                ((reg_value.u.bit.msc)?"1-1.8v":"0-3.3v"),reg_value.u.bit.ds,reg_value.u.bit.st,\
                reg_value.u.bit.sl,reg_value.u.bit.di);
    }
    return str;
}

// pin_num : 0-63 
static void machine_fpioa_help_print_pin_func(int pin_num, int detail_flag)
{
    int i = 0;
    char str_tmp[TEMP_STR_LEN];
    char str_tmp1[TEMP_STR_LEN];
    
    if(pin_num == -1){
        for(i = 0; i < MAX_PIN_NUM; i++){
            machine_fpioa_help_print_pin_func(i, detail_flag);
        }
        return;
    }
    memset(str_tmp, 0, sizeof(str_tmp));
    memset(str_tmp1, 0, sizeof(str_tmp1));
    fpioa_get_pin_funcs_str(pin_num, str_tmp, sizeof(str_tmp));
    fpioa_get_pin_cur_func_str(pin_num, str_tmp1, sizeof(str_tmp1), detail_flag);

    if(detail_flag){
        mp_printf(&mp_plat_print, "|%-17s|%-60s|\r\n","current config", str_tmp1);
        mp_printf(&mp_plat_print, "|%-17s|%-60s|\r\n","can be function", str_tmp);
    } else {
        mp_printf(&mp_plat_print, "| %-2d   | %-10s | %-56s|\r\n",pin_num, str_tmp1,  str_tmp);
    }
}


// 0---ok,  other --failed
static int fpioa_drv_pin_set(uint32_t pin , mp_arg_val_t *args)
{
    struct st_iomux_reg reg_value,org_value;
    int io_sel = 0;

    int func = args[ARG_func].u_int;
	int16_t set_st = args[ARG_set_st].u_int;
    int16_t set_ds = args[ARG_set_ds].u_int;
    int16_t set_pd = args[ARG_set_pd].u_int;
    int16_t set_pu = args[ARG_set_pu].u_int;
    int16_t set_oe = args[ARG_set_oe].u_int;
    int16_t set_ie = args[ARG_set_ie].u_int;
    int16_t set_msc = args[ARG_set_msc].u_int;
    int16_t set_sl = args[ARG_set_sl].u_int;
    int16_t set_di = args[ARG_set_di].u_int;

    //printf("st %d %d %d %d %d  %d %d %d %d %d \n",set_st,set_ds,set_pd,set_pu,set_oe,set_ie,set_msc,set_sl,set_di, func);

    if(pin >= MAX_PIN_NUM)
        return -2;

    if(fpioa_drv_reg_get(pin, (uint32_t*)&reg_value))
        return -3;
    org_value.u.value = reg_value.u.value;

    if(func != -1){
        io_sel = fpioa_drv_func_2_pin_io_sel(func, pin);
        reg_value.u.bit.io_sel = io_sel;
        if(io_sel < 0)
            return -4;
    }

    if(set_st != -1){
        if( (set_st < 0 ) || (set_st > 1 ))
            return -5;
        reg_value.u.bit.st = set_st;
    }
    
    if(set_ds != -1){
        if( (set_ds < 0) || (set_ds > 7 ))
            return -6;
        reg_value.u.bit.ds = set_ds;
    }

    if(set_pd != -1){
        if( (set_pd < 0) || (set_pd > 1 ))
            return -7;
        reg_value.u.bit.pd = set_pd;
    }

    if(set_pu != -1){
        if( (set_pu < 0) || (set_pu > 1 ))
            return -8;
        reg_value.u.bit.pu = set_pu;
    }

    if(set_oe != -1){
        if( (set_oe < 0) || (set_oe > 1 ))
            return -9;
        reg_value.u.bit.oe = set_oe;
    }


    if(set_ie != -1){
        if( (set_ie < 0) || (set_ie > 1 ))
            return -10;
        reg_value.u.bit.ie = set_ie;
    }

    if(set_msc != -1){
        // if( (set_msc < 0) || (set_msc > 1 ))
        //     return -11;
        //reg_value.u.bit.msc = set_msc;
        return -100;
    }

    if(set_sl != -1){
        if( (set_sl < 0) || (set_sl > 1 ))
            return -12;
        reg_value.u.bit.sl = set_sl;
    }

    if(set_di != -1){
        if( (set_di < 0) || (set_di > 1 ))
            return -14;
        reg_value.u.bit.di = set_di;
    }
    
    
    if(reg_value.u.value != org_value.u.value){
        if(fpioa_drv_reg_set(pin, reg_value.u.value))
            return -15;
    }
       
    return 0;
}

STATIC mp_obj_t machine_fpioa_set_function(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) 
{
    int ret;
    char str_tmp[TEMP_STR_LEN]={0};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_pin, 	MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = -1} },
		{ MP_QSTR_func,	MP_ARG_INT, {.u_int = -1} },
		{ MP_QSTR_set_st,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_ds,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_pd,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_pu,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_oe,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_ie,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_msc,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_sl,	MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_set_di,	MP_ARG_INT, {.u_int = -1} },		
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	uint16_t pin_num = args[ARG_pin].u_int;
	


    //printf("%x %x %x %x  %x \n",pin_num, func_num, set_sl, set_st, set_io_driving);

    if(pin_num >= MAX_PIN_NUM)
        mp_raise_msg_varg(&mp_type_IndexError, MP_ERROR_TEXT("pin num %d error ,need 0-%d"),pin_num, MAX_PIN_NUM-1);

    ret = fpioa_drv_pin_set(pin_num, args);

    //printf("ret =%d \n", ret);

    if(ret == -4)
        mp_raise_msg_varg(&mp_type_IndexError, \
                MP_ERROR_TEXT("pin num %d have not this func,can by "),pin_num, fpioa_get_pin_funcs_str(pin_num, str_tmp, sizeof(str_tmp)));
    else if(ret == -100){
        mp_raise_msg_varg(&mp_type_KeyError, MP_ERROR_TEXT("msc not support modify"));
    } else if( (ret >= -14 ) && (ret <= -5))
        mp_raise_msg_varg(&mp_type_IndexError, MP_ERROR_TEXT("parameter error"));
    else   if(ret){
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("system error,pleade retry"));
    }
   
	return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_fpioa_set_function_obj, 1, machine_fpioa_set_function);

STATIC mp_obj_t machine_get_Pin_num(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) 
{
    uint16_t func = -1;
    uint8_t pins[MAX_PIN_NUM];
    int pin_count = 0;

    if(n_args > 1){
        func = mp_obj_get_int(pos_args[1]);
    }
    pin_count = fpioa_drv_get_pin_from_func(func, pins);

    if(pin_count < -1)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("iomux pin reg read error "));

    mp_obj_t list = mp_obj_new_list(0, NULL);
    for(;pin_count >0;pin_count--)
        mp_obj_list_append(list, mp_obj_new_int(pins[pin_count-1]));

    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_fpioa_get_Pin_num_obj, 0, machine_get_Pin_num);

STATIC  mp_obj_t machine_fpioa_help(size_t n_args, const mp_obj_t *args) 
{
    int pin_num = -1;
    if(n_args > 1){
        pin_num = mp_obj_get_int(args[1]);
        if( (pin_num >= MAX_PIN_NUM) || (pin_num < -1)){
            mp_raise_msg_varg(&mp_type_IndexError, MP_ERROR_TEXT("pin num %d error ,need 0-%d"),pin_num, MAX_PIN_NUM-1);
        }
        mp_printf(&mp_plat_print, "|%-17s|%-60d|\r\n","pin num ",pin_num);
        machine_fpioa_help_print_pin_func(pin_num, 1);
    }else {
        mp_printf(&mp_plat_print, "| pin  | cur func   |                can be func                              |\r\n") ;
        mp_printf(&mp_plat_print, "| ---- |------------|---------------------------------------------------------|\r\n") ;
        machine_fpioa_help_print_pin_func(pin_num, 0);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_fpioa_help_obj, 1,2,machine_fpioa_help);

STATIC void mp_machine_gpio_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_fpioa_help(1, NULL);
    return ;
}
STATIC mp_obj_t mp_machine_fpioa_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_fpioa_obj_t *self = mp_obj_malloc(machine_fpioa_obj_t, &machine_fpioa_type);
    return MP_OBJ_FROM_PTR(self);
}
STATIC const mp_rom_map_elem_t machine_fpioa_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_function), MP_ROM_PTR(&machine_fpioa_set_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_Pin_num), MP_ROM_PTR(&machine_fpioa_get_Pin_num_obj) },
    { MP_ROM_QSTR(MP_QSTR_help), MP_ROM_PTR(&machine_fpioa_help_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO0 ),  MP_ROM_INT(GPIO0 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO1 ),  MP_ROM_INT(GPIO1 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO2 ),  MP_ROM_INT(GPIO2 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO3 ),  MP_ROM_INT(GPIO3 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO4 ),  MP_ROM_INT(GPIO4 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO5 ),  MP_ROM_INT(GPIO5 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO6 ),  MP_ROM_INT(GPIO6 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO7 ),  MP_ROM_INT(GPIO7 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO8 ),  MP_ROM_INT(GPIO8 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO9 ),  MP_ROM_INT(GPIO9 ) },
    { MP_ROM_QSTR(MP_QSTR_GPIO10),  MP_ROM_INT(GPIO10) },
    { MP_ROM_QSTR(MP_QSTR_GPIO11),  MP_ROM_INT(GPIO11) },
    { MP_ROM_QSTR(MP_QSTR_GPIO12),  MP_ROM_INT(GPIO12) },
    { MP_ROM_QSTR(MP_QSTR_GPIO13),  MP_ROM_INT(GPIO13) },
    { MP_ROM_QSTR(MP_QSTR_GPIO14),  MP_ROM_INT(GPIO14) },
    { MP_ROM_QSTR(MP_QSTR_GPIO15),  MP_ROM_INT(GPIO15) },
    { MP_ROM_QSTR(MP_QSTR_GPIO16),  MP_ROM_INT(GPIO16) },
    { MP_ROM_QSTR(MP_QSTR_GPIO17),  MP_ROM_INT(GPIO17) },
    { MP_ROM_QSTR(MP_QSTR_GPIO18),  MP_ROM_INT(GPIO18) },
    { MP_ROM_QSTR(MP_QSTR_GPIO19),  MP_ROM_INT(GPIO19) },
    { MP_ROM_QSTR(MP_QSTR_GPIO20),  MP_ROM_INT(GPIO20) },
    { MP_ROM_QSTR(MP_QSTR_GPIO21),  MP_ROM_INT(GPIO21) },
    { MP_ROM_QSTR(MP_QSTR_GPIO22),  MP_ROM_INT(GPIO22) },
    { MP_ROM_QSTR(MP_QSTR_GPIO23),  MP_ROM_INT(GPIO23) },
    { MP_ROM_QSTR(MP_QSTR_GPIO24),  MP_ROM_INT(GPIO24) },
    { MP_ROM_QSTR(MP_QSTR_GPIO25),  MP_ROM_INT(GPIO25) },
    { MP_ROM_QSTR(MP_QSTR_GPIO26),  MP_ROM_INT(GPIO26) },
    { MP_ROM_QSTR(MP_QSTR_GPIO27),  MP_ROM_INT(GPIO27) },
    { MP_ROM_QSTR(MP_QSTR_GPIO28),  MP_ROM_INT(GPIO28) },
    { MP_ROM_QSTR(MP_QSTR_GPIO29),  MP_ROM_INT(GPIO29) },
    { MP_ROM_QSTR(MP_QSTR_GPIO30),  MP_ROM_INT(GPIO30) },
    { MP_ROM_QSTR(MP_QSTR_GPIO31),  MP_ROM_INT(GPIO31) },
    { MP_ROM_QSTR(MP_QSTR_GPIO32),  MP_ROM_INT(GPIO32) },
    { MP_ROM_QSTR(MP_QSTR_GPIO33),  MP_ROM_INT(GPIO33) },
    { MP_ROM_QSTR(MP_QSTR_GPIO34),  MP_ROM_INT(GPIO34) },
    { MP_ROM_QSTR(MP_QSTR_GPIO35),  MP_ROM_INT(GPIO35) },
    { MP_ROM_QSTR(MP_QSTR_GPIO36),  MP_ROM_INT(GPIO36) },
    { MP_ROM_QSTR(MP_QSTR_GPIO37),  MP_ROM_INT(GPIO37) },
    { MP_ROM_QSTR(MP_QSTR_GPIO38),  MP_ROM_INT(GPIO38) },
    { MP_ROM_QSTR(MP_QSTR_GPIO39),  MP_ROM_INT(GPIO39) },
    { MP_ROM_QSTR(MP_QSTR_GPIO40),  MP_ROM_INT(GPIO40) },
    { MP_ROM_QSTR(MP_QSTR_GPIO41),  MP_ROM_INT(GPIO41) },
    { MP_ROM_QSTR(MP_QSTR_GPIO42),  MP_ROM_INT(GPIO42) },
    { MP_ROM_QSTR(MP_QSTR_GPIO43),  MP_ROM_INT(GPIO43) },
    { MP_ROM_QSTR(MP_QSTR_GPIO44),  MP_ROM_INT(GPIO44) },
    { MP_ROM_QSTR(MP_QSTR_GPIO45),  MP_ROM_INT(GPIO45) },
    { MP_ROM_QSTR(MP_QSTR_GPIO46),  MP_ROM_INT(GPIO46) },
    { MP_ROM_QSTR(MP_QSTR_GPIO47),  MP_ROM_INT(GPIO47) },
    { MP_ROM_QSTR(MP_QSTR_GPIO48),  MP_ROM_INT(GPIO48) },
    { MP_ROM_QSTR(MP_QSTR_GPIO49),  MP_ROM_INT(GPIO49) },
    { MP_ROM_QSTR(MP_QSTR_GPIO50),  MP_ROM_INT(GPIO50) },
    { MP_ROM_QSTR(MP_QSTR_GPIO51),  MP_ROM_INT(GPIO51) },
    { MP_ROM_QSTR(MP_QSTR_GPIO52),  MP_ROM_INT(GPIO52) },
    { MP_ROM_QSTR(MP_QSTR_GPIO53),  MP_ROM_INT(GPIO53) },
    { MP_ROM_QSTR(MP_QSTR_GPIO54),  MP_ROM_INT(GPIO54) },
    { MP_ROM_QSTR(MP_QSTR_GPIO55),  MP_ROM_INT(GPIO55) },
    { MP_ROM_QSTR(MP_QSTR_GPIO56),  MP_ROM_INT(GPIO56) },
    { MP_ROM_QSTR(MP_QSTR_GPIO57),  MP_ROM_INT(GPIO57) },
    { MP_ROM_QSTR(MP_QSTR_GPIO58),  MP_ROM_INT(GPIO58) },
    { MP_ROM_QSTR(MP_QSTR_GPIO59),  MP_ROM_INT(GPIO59) },
    { MP_ROM_QSTR(MP_QSTR_GPIO60),  MP_ROM_INT(GPIO60) },
    { MP_ROM_QSTR(MP_QSTR_GPIO61),  MP_ROM_INT(GPIO61) },
    { MP_ROM_QSTR(MP_QSTR_GPIO62),  MP_ROM_INT(GPIO62) },
    { MP_ROM_QSTR(MP_QSTR_GPIO63),  MP_ROM_INT(GPIO63) },
    { MP_ROM_QSTR(MP_QSTR_BOOT0),  MP_ROM_INT(BOOT0) },
    { MP_ROM_QSTR(MP_QSTR_BOOT1),  MP_ROM_INT(BOOT1) },
    { MP_ROM_QSTR(MP_QSTR_CI0),  MP_ROM_INT(CI0) },
    { MP_ROM_QSTR(MP_QSTR_CI1),  MP_ROM_INT(CI1) },
    { MP_ROM_QSTR(MP_QSTR_CI2),  MP_ROM_INT(CI2) },
    { MP_ROM_QSTR(MP_QSTR_CI3),  MP_ROM_INT(CI3) },
    { MP_ROM_QSTR(MP_QSTR_CO0),  MP_ROM_INT(CO0) },
    { MP_ROM_QSTR(MP_QSTR_CO1),  MP_ROM_INT(CO1) },
    { MP_ROM_QSTR(MP_QSTR_CO2),  MP_ROM_INT(CO2) },
    { MP_ROM_QSTR(MP_QSTR_CO3),  MP_ROM_INT(CO3) },
    { MP_ROM_QSTR(MP_QSTR_DI0),  MP_ROM_INT(DI0) },
    { MP_ROM_QSTR(MP_QSTR_DI1),  MP_ROM_INT(DI1) },
    { MP_ROM_QSTR(MP_QSTR_DI2),  MP_ROM_INT(DI2) },
    { MP_ROM_QSTR(MP_QSTR_DI3),  MP_ROM_INT(DI3) },
    { MP_ROM_QSTR(MP_QSTR_DO0),  MP_ROM_INT(DO0) },
    { MP_ROM_QSTR(MP_QSTR_DO1),  MP_ROM_INT(DO1) },
    { MP_ROM_QSTR(MP_QSTR_DO2),  MP_ROM_INT(DO2) },
    { MP_ROM_QSTR(MP_QSTR_DO3),  MP_ROM_INT(DO3) },
    { MP_ROM_QSTR(MP_QSTR_HSYNC0),  MP_ROM_INT(HSYNC0) },
    { MP_ROM_QSTR(MP_QSTR_HSYNC1),  MP_ROM_INT(HSYNC1) },
    { MP_ROM_QSTR(MP_QSTR_IIC0_SCL),  MP_ROM_INT(IIC0_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC0_SDA),  MP_ROM_INT(IIC0_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC1_SCL),  MP_ROM_INT(IIC1_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC1_SDA),  MP_ROM_INT(IIC1_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC2_SCL),  MP_ROM_INT(IIC2_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC2_SDA),  MP_ROM_INT(IIC2_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC3_SCL),  MP_ROM_INT(IIC3_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC3_SDA),  MP_ROM_INT(IIC3_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIC4_SCL),  MP_ROM_INT(IIC4_SCL) },
    { MP_ROM_QSTR(MP_QSTR_IIC4_SDA),  MP_ROM_INT(IIC4_SDA) },
    { MP_ROM_QSTR(MP_QSTR_IIS_CLK),  MP_ROM_INT(IIS_CLK) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_IN0),  MP_ROM_INT(IIS_D_IN0) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_IN1),  MP_ROM_INT(IIS_D_IN1) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_OUT0),  MP_ROM_INT(IIS_D_OUT0) },
    { MP_ROM_QSTR(MP_QSTR_IIS_D_OUT1),  MP_ROM_INT(IIS_D_OUT1) },
    { MP_ROM_QSTR(MP_QSTR_IIS_WS),  MP_ROM_INT(IIS_WS) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_RST),  MP_ROM_INT(JTAG_RST) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TCK),  MP_ROM_INT(JTAG_TCK) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TDI),  MP_ROM_INT(JTAG_TDI) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TDO),  MP_ROM_INT(JTAG_TDO) },
    { MP_ROM_QSTR(MP_QSTR_JTAG_TMS),  MP_ROM_INT(JTAG_TMS) },
    { MP_ROM_QSTR(MP_QSTR_M_CLK1),  MP_ROM_INT(M_CLK1) },
    { MP_ROM_QSTR(MP_QSTR_M_CLK2),  MP_ROM_INT(M_CLK2) },
    { MP_ROM_QSTR(MP_QSTR_M_CLK3),  MP_ROM_INT(M_CLK3) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_CLK),  MP_ROM_INT(MMC1_CLK) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_CMD),  MP_ROM_INT(MMC1_CMD) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D0),  MP_ROM_INT(MMC1_D0) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D1),  MP_ROM_INT(MMC1_D1) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D2),  MP_ROM_INT(MMC1_D2) },
    { MP_ROM_QSTR(MP_QSTR_MMC1_D3),  MP_ROM_INT(MMC1_D3) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_CLK),  MP_ROM_INT(OSPI_CLK) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_CS),  MP_ROM_INT(OSPI_CS) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D0),  MP_ROM_INT(OSPI_D0) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D1),  MP_ROM_INT(OSPI_D1) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D2),  MP_ROM_INT(OSPI_D2) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D3),  MP_ROM_INT(OSPI_D3) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D4),  MP_ROM_INT(OSPI_D4) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D5),  MP_ROM_INT(OSPI_D5) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D6),  MP_ROM_INT(OSPI_D6) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_D7),  MP_ROM_INT(OSPI_D7) },
    { MP_ROM_QSTR(MP_QSTR_OSPI_DQS),  MP_ROM_INT(OSPI_DQS) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN0),  MP_ROM_INT(PDM_IN0) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN1),  MP_ROM_INT(PDM_IN1) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN2),  MP_ROM_INT(PDM_IN2) },
    { MP_ROM_QSTR(MP_QSTR_PDM_IN3),  MP_ROM_INT(PDM_IN3) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR0),  MP_ROM_INT(PULSE_CNTR0) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR1),  MP_ROM_INT(PULSE_CNTR1) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR2),  MP_ROM_INT(PULSE_CNTR2) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR3),  MP_ROM_INT(PULSE_CNTR3) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR4),  MP_ROM_INT(PULSE_CNTR4) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_CNTR5),  MP_ROM_INT(PULSE_CNTR5) },
    { MP_ROM_QSTR(MP_QSTR_PWM0),  MP_ROM_INT(PWM0) },
    { MP_ROM_QSTR(MP_QSTR_PWM1),  MP_ROM_INT(PWM1) },
    { MP_ROM_QSTR(MP_QSTR_PWM2),  MP_ROM_INT(PWM2) },
    { MP_ROM_QSTR(MP_QSTR_PWM3),  MP_ROM_INT(PWM3) },
    { MP_ROM_QSTR(MP_QSTR_PWM4),  MP_ROM_INT(PWM4) },
    { MP_ROM_QSTR(MP_QSTR_PWM5),  MP_ROM_INT(PWM5) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CLK),  MP_ROM_INT(QSPI0_CLK) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS0),  MP_ROM_INT(QSPI0_CS0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS1),  MP_ROM_INT(QSPI0_CS1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS2),  MP_ROM_INT(QSPI0_CS2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS3),  MP_ROM_INT(QSPI0_CS3) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_CS4),  MP_ROM_INT(QSPI0_CS4) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D0),  MP_ROM_INT(QSPI0_D0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D1),  MP_ROM_INT(QSPI0_D1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D2),  MP_ROM_INT(QSPI0_D2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI0_D3),  MP_ROM_INT(QSPI0_D3) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CLK),  MP_ROM_INT(QSPI1_CLK) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS0),  MP_ROM_INT(QSPI1_CS0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS1),  MP_ROM_INT(QSPI1_CS1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS2),  MP_ROM_INT(QSPI1_CS2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS3),  MP_ROM_INT(QSPI1_CS3) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_CS4),  MP_ROM_INT(QSPI1_CS4) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D0),  MP_ROM_INT(QSPI1_D0) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D1),  MP_ROM_INT(QSPI1_D1) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D2),  MP_ROM_INT(QSPI1_D2) },
    { MP_ROM_QSTR(MP_QSTR_QSPI1_D3),  MP_ROM_INT(QSPI1_D3) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_CK),  MP_ROM_INT(SPI2AXI_CK) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_CS),  MP_ROM_INT(SPI2AXI_CS) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_DI),  MP_ROM_INT(SPI2AXI_DI) },
    { MP_ROM_QSTR(MP_QSTR_SPI2AXI_DO),  MP_ROM_INT(SPI2AXI_DO) },
    { MP_ROM_QSTR(MP_QSTR_UART0_RXD),  MP_ROM_INT(UART0_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART0_TXD),  MP_ROM_INT(UART0_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART1_CTS),  MP_ROM_INT(UART1_CTS) },
    { MP_ROM_QSTR(MP_QSTR_UART1_RTS),  MP_ROM_INT(UART1_RTS) },
    { MP_ROM_QSTR(MP_QSTR_UART1_RXD),  MP_ROM_INT(UART1_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART1_TXD),  MP_ROM_INT(UART1_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART2_CTS),  MP_ROM_INT(UART2_CTS) },
    { MP_ROM_QSTR(MP_QSTR_UART2_RTS),  MP_ROM_INT(UART2_RTS) },
    { MP_ROM_QSTR(MP_QSTR_UART2_RXD),  MP_ROM_INT(UART2_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART2_TXD),  MP_ROM_INT(UART2_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART3_CTS),  MP_ROM_INT(UART3_CTS) },
    { MP_ROM_QSTR(MP_QSTR_UART3_DE),  MP_ROM_INT(UART3_DE) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RE),  MP_ROM_INT(UART3_RE) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RTS),  MP_ROM_INT(UART3_RTS) },
    { MP_ROM_QSTR(MP_QSTR_UART3_RXD),  MP_ROM_INT(UART3_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART3_TXD),  MP_ROM_INT(UART3_TXD) },
    { MP_ROM_QSTR(MP_QSTR_UART4_RXD),  MP_ROM_INT(UART4_RXD) },
    { MP_ROM_QSTR(MP_QSTR_UART4_TXD),  MP_ROM_INT(UART4_TXD) },
    { MP_ROM_QSTR(MP_QSTR_PDM_CLK),  MP_ROM_INT(PDM_CLK) },
    { MP_ROM_QSTR(MP_QSTR_VSYNC0),  MP_ROM_INT(VSYNC0) },
    { MP_ROM_QSTR(MP_QSTR_VSYNC1),  MP_ROM_INT(VSYNC1) },
    { MP_ROM_QSTR(MP_QSTR_CTRL_IN_3D),  MP_ROM_INT(CTRL_IN_3D) },
    { MP_ROM_QSTR(MP_QSTR_CTRL_O1_3D),  MP_ROM_INT(CTRL_O1_3D) },
    { MP_ROM_QSTR(MP_QSTR_CTRL_O2_3D),  MP_ROM_INT(CTRL_O2_3D) },    
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN0 ),  MP_ROM_INT(TEST_PIN0 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN1 ),  MP_ROM_INT(TEST_PIN1 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN2 ),  MP_ROM_INT(TEST_PIN2 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN3 ),  MP_ROM_INT(TEST_PIN3 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN4 ),  MP_ROM_INT(TEST_PIN4 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN5 ),  MP_ROM_INT(TEST_PIN5 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN6 ),  MP_ROM_INT(TEST_PIN6 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN7 ),  MP_ROM_INT(TEST_PIN7 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN8 ),  MP_ROM_INT(TEST_PIN8 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN9 ),  MP_ROM_INT(TEST_PIN9 ) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN10),  MP_ROM_INT(TEST_PIN10) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN11),  MP_ROM_INT(TEST_PIN11) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN12),  MP_ROM_INT(TEST_PIN12) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN13),  MP_ROM_INT(TEST_PIN13) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN14),  MP_ROM_INT(TEST_PIN14) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN15),  MP_ROM_INT(TEST_PIN15) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN16),  MP_ROM_INT(TEST_PIN16) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN17),  MP_ROM_INT(TEST_PIN17) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN18),  MP_ROM_INT(TEST_PIN18) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN19),  MP_ROM_INT(TEST_PIN19) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN20),  MP_ROM_INT(TEST_PIN20) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN21),  MP_ROM_INT(TEST_PIN21) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN22),  MP_ROM_INT(TEST_PIN22) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN23),  MP_ROM_INT(TEST_PIN23) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN24),  MP_ROM_INT(TEST_PIN24) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN25),  MP_ROM_INT(TEST_PIN25) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN26),  MP_ROM_INT(TEST_PIN26) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN27),  MP_ROM_INT(TEST_PIN27) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN28),  MP_ROM_INT(TEST_PIN28) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN29),  MP_ROM_INT(TEST_PIN29) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN30),  MP_ROM_INT(TEST_PIN30) },
    { MP_ROM_QSTR(MP_QSTR_TEST_PIN31),  MP_ROM_INT(TEST_PIN31) },
};
STATIC MP_DEFINE_CONST_DICT(machine_fpioa_locals_dict, machine_fpioa_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_fpioa_type,
    MP_QSTR_FPIOA,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_fpioa_make_new,
    print, mp_machine_gpio_print,
    locals_dict, &machine_fpioa_locals_dict
    );