/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_SPI_H_
#define _YOS_SPI_H_

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FPCLK					(84 * 1000 * 1000)

/* This is for CS pin to SPI device */
enum YSPI_CS_VALID_VALUE {
	YSPI_CS_VALID_ON_LOW,
	YSPI_CS_VALID_ON_HIGH
};

#define YSPI_FREQUENCY_MAX				(DEFAULT_FPCLK / 2)
#define YSPI_FREQUENCY_MIN				(YSPI_FREQUENCY_MAX / 128)

enum YSPI_DEVICE_EVENT {
	YSPI_DEVICE_EVENT_INVALID = -1,
	YSPI_DEVICE_EVENT_TX_COMPLETE,
	YSPI_DEVICE_EVENT_TX_HALF_TRANSFERED,
	YSPI_DEVICE_EVENT_TX_ERROR,
	YSPI_DEVICE_EVENT_TX_DIRECT_ERROR,
	YSPI_DEVICE_EVENT_TX_FIFO_ERROR,

	YSPI_DEVICE_EVENT_RX_COMPLETE,
	YSPI_DEVICE_EVENT_RX_HALF_TRANSFERED,
	YSPI_DEVICE_EVENT_RX_ERROR,
	YSPI_DEVICE_EVENT_RX_DIRECT_ERROR,
	YSPI_DEVICE_EVENT_RX_FIFO_ERROR,

	YSPI_DEVICE_EVENT_MAX
};

struct yspi_device {
	uint32_t cs_gpio_port;
	uint16_t cs_gpio_num;
	enum YSPI_CS_VALID_VALUE valid_value;

	int is_in_transaction;
	void (*on_event)(enum YSPI_DEVICE_EVENT evt);
};

/*
 * Init a cs pin with port-number and its valid value.
 * Note that the GPIO port must be initialized in advance.
 *
 * Return 0 if succeeds, other value means init failure.
 */
int yspi_device_init(struct yspi_device *dev, uint32_t gpio_port,
					uint16_t gpio_num, enum YSPI_CS_VALID_VALUE valid_value,
					void (*on_event)(enum YSPI_DEVICE_EVENT evt));

/*
 * Deinit a yspi device
 *
 * Return 0 if succeeds, other value means init failure.
 */
int yspi_device_deinit(struct yspi_device *dev);

int yspi_device_select(struct yspi_device *dev);
int yspi_device_unselect(struct yspi_device *dev);


int yspi_master_init(void);
int yspi_master_deinit(void);

int yspi_master_set_speed(uint32_t freq);

int yspi_trans_begin(struct yspi_device *cs);
int yspi_trans_end(struct yspi_device *cs);
uint32_t yspi_trans_send(void *data, uint32_t data_len);
uint32_t yspi_trans_receive(void *buf, uint32_t buf_len);
uint8_t yspi_trans_write_and_read(struct yspi_device *cs, uint8_t data);

uint8_t yspi_write_and_read(struct yspi_device *cs, uint8_t data);

#ifdef __cplusplus
}
#endif
#endif
