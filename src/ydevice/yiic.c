/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <stdio.h>
#include "../yos/common_def.h"
#include "../yos/ymutex.h"
#include "../yos/yos.h"
#include "yiic.h"

#define DEFAULT_IIC							I2C1
#define DEFAULT_IIC_RCC						RCC_I2C1
#define DEFAULT_IIC_RCC_RST					RST_I2C1
#define DEFAULT_IIC_GPIO_RCC				RCC_GPIOB
#define DEFAULT_IIC_GPIO_BANK				GPIOB
#define DEFAULT_IIC_GPIO_SCL				GPIO6
#define DEFAULT_IIC_GPIO_SDA				GPIO7
#define DEFAULT_IIC_CLOCK_FREQ_MHZ			(rcc_apb1_frequency / (1000 * 1000))

/*
 * IIC Speed
 * 0:		Standard Speed(~100KHz)
 * 1:		Fast Speed(~400KHz)
 */
#define DEFAULE_IIC_SPEED					1

static struct ymutex _yiic_mutex;

int yiic_master_init(uint32_t iic_freq)
{
#if 0
	if (iic_freq == 0) {
		return -1;
	}
#endif

	/* Enable clocks for GPIO and I2C */
	rcc_periph_clock_enable(DEFAULT_IIC_RCC);
	rcc_periph_clock_enable(DEFAULT_IIC_GPIO_RCC);

	/* GPIO setup for SCL and SDA */
	gpio_set_output_options(DEFAULT_IIC_GPIO_BANK, GPIO_OTYPE_OD,
				GPIO_OSPEED_50MHZ, DEFAULT_IIC_GPIO_SCL | DEFAULT_IIC_GPIO_SDA);
	gpio_mode_setup(DEFAULT_IIC_GPIO_BANK, GPIO_MODE_AF,
				GPIO_PUPD_NONE, DEFAULT_IIC_GPIO_SCL | DEFAULT_IIC_GPIO_SDA);
	gpio_set_af(DEFAULT_IIC_GPIO_BANK, GPIO_AF4, DEFAULT_IIC_GPIO_SCL | DEFAULT_IIC_GPIO_SDA);

	/* I2C setup */
	i2c_peripheral_disable(DEFAULT_IIC);

#if (DEFAULE_IIC_SPEED == 0)
	/* Standard Speed(~100 KHz) */
	i2c_set_speed(DEFAULT_IIC, i2c_speed_sm_100k, DEFAULT_IIC_CLOCK_FREQ_MHZ);
#elif (DEFAULE_IIC_SPEED == 1)
	/* Fast Speed(~400 KHz) */
	i2c_set_speed(DEFAULT_IIC, i2c_speed_fm_400k, DEFAULT_IIC_CLOCK_FREQ_MHZ);
#else
	#error "Wrong IIC speed setting, please set it properly"
#endif

	i2c_peripheral_enable(DEFAULT_IIC);

	ymutex_init(&_yiic_mutex);

	return 0;
}

int yiic_master_deinit()
{
	ymutex_deinit(&_yiic_mutex);
	i2c_peripheral_disable(DEFAULT_IIC);
	rcc_periph_clock_disable(DEFAULT_IIC_RCC);

	return 0;
}

void yiic_master_wait(void)
{
	while ( !( (I2C_SR1(DEFAULT_IIC) & I2C_SR1_SB)
			&& (I2C_SR2(DEFAULT_IIC) & I2C_SR2_MSL)
			&& (I2C_SR2(DEFAULT_IIC) & I2C_SR2_BUSY) )) {
	}
}

static uint8_t volatile addr_for_current_trans = 0;
static enum yiic_master_tr volatile current_tran_type = YIIC_MASTER_TRANSMIT;
int yiic_master_trans_start(enum yiic_master_tr tran_type)
{
	ymutex_lock(&_yiic_mutex);

	current_tran_type = tran_type;
	if (current_tran_type == YIIC_MASTER_TRANSMIT) {
		while ((I2C_SR2(DEFAULT_IIC) & I2C_SR2_BUSY)) {
		}
	}

	i2c_send_start(DEFAULT_IIC);

	if (current_tran_type == YIIC_MASTER_RECEIVE) {
		i2c_enable_ack(DEFAULT_IIC);
	}

	return 0;
}

int yiic_master_trans_stop(void)
{
	i2c_send_stop(DEFAULT_IIC);

	ymutex_unlock(&_yiic_mutex);

	return 0;
}

int yiic_master_select_slave(uint8_t addr)
{
	uint32_t tmp_sr2;
	yiic_master_wait();

	i2c_send_7bit_address(DEFAULT_IIC, addr, current_tran_type == YIIC_MASTER_TRANSMIT ? I2C_WRITE : I2C_READ);
	while (!(I2C_SR1(DEFAULT_IIC) & I2C_SR1_ADDR)) {
	}
	tmp_sr2 = I2C_SR2(DEFAULT_IIC);

	return 0;
}

uint16_t yiic_master_transmit_data(void *data, uint16_t data_len)
{
	uint16_t bytes_transmited = 0;
	if (data == NULL) {
		goto para_err;
	}

	while (bytes_transmited < data_len) {
		i2c_send_data(DEFAULT_IIC, *((uint8_t *)data + bytes_transmited));
		while (!(I2C_SR1(DEFAULT_IIC) & I2C_SR1_BTF)) {
		}

		bytes_transmited++;
	}

para_err:
	return bytes_transmited;
}

uint16_t yiic_master_receive_data(void *buff, uint16_t buf_len)
{
	uint16_t bytes_received = 0;
	if (buff == NULL) {
		goto para_err;
	}

	int is_last_byte;
	while (bytes_received < buf_len) {
		is_last_byte = (bytes_received == buf_len - 1);

		if (is_last_byte) {
			/* Last byte */
			i2c_disable_ack(DEFAULT_IIC);
		} else {
			/* Not last byte */
		}

		while (!(I2C_SR1(DEFAULT_IIC) & I2C_SR1_RxNE)) {
		}

		*((uint8_t *)buff + bytes_received) = i2c_get_data(DEFAULT_IIC);

		bytes_received++;
	}

para_err:
	return bytes_received;
}
