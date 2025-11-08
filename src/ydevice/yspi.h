/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_SPI_H_
#define _YOS_SPI_H_

#include <stdarg.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FPCLK					(84 * 1000 * 1000)

#define DEFAULT_SPI_TRAN_WITH_MUTEX		1
#define DEFAULT_SPI_USE_DMA				0

#define DEFAULT_SPI_OUTPUT_DBG_MSG		0

#if (DEFAULT_SPI_USE_DMA == 1)
#define DEFAULT_SPI_DMA_RX_BUFFER_SIZE	4096
#define DEFAULT_SPI_DMA_TX_BUFFER_SIZE	4096

#if (DEFAULT_SPI_DMA_RX_BUFFER_SIZE != DEFAULT_SPI_DMA_TX_BUFFER_SIZE)
#error "SPI DMA buffer size for RX and TX must be equal"
#endif
#endif


/* SPI GPIO Settings */
#define DEFAULT_SPI						SPI1
#define DEFAULT_SPI_RCC					RCC_SPI1
#define DEFAULT_SPI_GPIO_RCC			RCC_GPIOA
#define DEFAULT_SPI_GPIO_PORT			GPIOA
#define DEFAULT_SPI_GPIO_SCK			GPIO5
#define DEFAULT_SPI_GPIO_MISO			GPIO6
#define DEFAULT_SPI_GPIO_MOSI			GPIO7
#define DEFAULT_SPI_NVIC_IRQ			NVIC_SPI1_IRQ
#define DEFAULT_SPI_ISR_FUNC			spi1_isr


/* SPI Settings */
#define DEFAULT_SPI_BAUDRATE			SPI_CR1_BAUDRATE_FPCLK_DIV_2
#define DEFAULT_SPI_CPOL				SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE
#define DEFAULT_SPI_CPHA				SPI_CR1_CPHA_CLK_TRANSITION_1
#define DEFAULT_SPI_DATA_FMT			SPI_CR1_DFF_8BIT
#define DEFAULT_SPI_MSB_LSB_FIRST		SPI_CR1_MSBFIRST


#if (DEFAULT_SPI_USE_DMA == 1)
#define DEFAULT_SPI_DMA_RCC				RCC_DMA2
#define DEFAULT_SPI_DMAD_RCC			RCC_DMA2D

/* SPI DMA Settings: RX */
#define DEFAULT_SPI_DMA_RX				DMA2
#define DEFAULT_SPI_DMA_RX_STREAM		DMA_STREAM2
#define DEFAULT_SPI_DMA_RX_CHANNEL		DMA_SxCR_CHSEL_3
#define DEFAULT_SPI_DMA_RX_MEM_SIZE		DMA_SxCR_MSIZE_8BIT
#define DEFAULT_SPI_DMA_RX_PERI_SIZE	DMA_SxCR_PSIZE_8BIT
#define DEFAULT_SPI_DMA_RX_PERI_ADDR	SPI1_DR
#define DEFAULT_SPI_DMA_RX_NVIC_IRQ		NVIC_DMA2_STREAM2_IRQ
#define DEFAULT_SPI_DMA_RX_ISR			dma2_stream2_isr

/* SPI DMA Settings: TX */
#define DEFAULT_SPI_DMA_TX				DMA2
#define DEFAULT_SPI_DMA_TX_STREAM		DMA_STREAM5
#define DEFAULT_SPI_DMA_TX_CHANNEL		DMA_SxCR_CHSEL_3
#define DEFAULT_SPI_DMA_TX_MEM_SIZE		DMA_SxCR_MSIZE_8BIT
#define DEFAULT_SPI_DMA_TX_PERI_SIZE	DMA_SxCR_PSIZE_8BIT
#define DEFAULT_SPI_DMA_TX_PERI_ADDR	SPI1_DR
#define DEFAULT_SPI_DMA_TX_NVIC_IRQ		NVIC_DMA2_STREAM5_IRQ
#define DEFAULT_SPI_DMA_TX_ISR			dma2_stream5_isr
#endif


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

uint8_t yspi_write_and_read_byte(uint8_t data);
uint32_t yspi_send(void *data, uint32_t data_len);
uint32_t yspi_receive(void *buf, uint32_t buf_len);
uint32_t yspi_send_and_receive(void *send_data, void *recv_buf, uint32_t data_len, uint8_t send_byte_filler);

uint8_t yspi_trans_write_and_read_byte(struct yspi_device *cs, uint8_t data);

#if (DEFAULT_SPI_OUTPUT_DBG_MSG == 1)
#include "../../lib/cmdline/basic_io.h"
#define YSPI_DBG(...)			basic_io_printf("[YSPI]" __VA_ARGS__)
#else
#define YSPI_DBG(...)
#endif

#ifdef __cplusplus
}
#endif
#endif
