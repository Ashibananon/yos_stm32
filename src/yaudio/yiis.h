/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_IIS_H_
#define _YOS_IIS_H_

#include <stdarg.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "../yos/common_def.h"
#include "../ydevice/yspi.h"
#include "../../lib/yrenga/yringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_IIS_OUTPUT_DBG_MSG				1

#define DEFAULT_IIS_USE_DMA						1

#define IIS_DMA_WAIT_STATICSTIC					1

#define DEFAULT_IIS_GPIO_RCC					RCC_GPIOB
#define DEFAULT_IIS_GPIO_PORT					GPIOB

#define DEFAULT_IIS_GPIO_WS						GPIO12
#define DEFAULT_IIS_GPIO_CLK					GPIO13
#define DEFAULT_IIS_GPIO_SD						GPIO15

#define DEFAULT_IIS_REC_GPIO_RCC_WS				RCC_GPIOA
#define DEFAULT_IIS_REC_GPIO_RCC_CLK_SD			RCC_GPIOB
#define DEFAULT_IIS_REC_GPIO_PORT_WS			GPIOA
#define DEFAULT_IIS_REC_GPIO_PORT_CLK_SD		GPIOB
#define DEFAULT_IIS_REC_GPIO_WS					GPIO15
#define DEFAULT_IIS_REC_GPIO_CLK				GPIO3
#define DEFAULT_IIS_REC_GPIO_SD					GPIO5

enum IIS_AUDIO_STANDARD {
	IIS_AUDIO_STANDARD_INVALID = -1,
	IIS_AUDIO_STANDARD_PHILIPS_STANDARD,
	IIS_AUDIO_STANDARD_MSB_JUSTIFIED,
	IIS_AUDIO_STANDARD_LSB_JUSTIFIED,
	IIS_AUDIO_STANDARD_PCM_STANDARD,
	IIS_AUDIO_STANDARD_MAX
};


struct yiis_ctrl {
	uint32_t iis_rcc;
	uint32_t iis_rcc_rst;
	uint32_t iis_base;
	uint32_t iis_nvic_irq;
	void (*iis_gpio_init)(int is_init);

#if (DEFAULT_IIS_USE_DMA == 1)
	uint32_t iis_dma_rx_rcc;
	uint32_t iis_dma_rx_base;
	uint32_t iis_dma_rx_stream;
	uint32_t iis_dma_rx_chsel;
	uint32_t iis_dma_rx_mem_size;
	uint32_t iis_dma_rx_peri_size;
	uint32_t iis_dma_rx_peri_addr;
	uint32_t iis_dma_rx_nvic_irq;
	int volatile iis_dma_rx_enabled;

	uint32_t iis_dma_tx_rcc;
	uint32_t iis_dma_tx_base;
	uint32_t iis_dma_tx_stream;
	uint32_t iis_dma_tx_chsel;
	uint32_t iis_dma_tx_mem_size;
	uint32_t iis_dma_tx_peri_size;
	uint32_t iis_dma_tx_peri_addr;
	uint32_t iis_dma_tx_nvic_irq;
	int volatile iis_dma_tx_enabled;

	struct YRingBuffer *dma_rx_ringbuf;
	struct YRingBuffer *dma_tx_ringbuf;

	int volatile is_dma_working;

	uint32_t sampling_rate;
	uint8_t channels;
	uint8_t bit_depth;
	uint8_t transfer_bit_width;
	enum IIS_AUDIO_STANDARD audio_standard;

#if (IIS_DMA_WAIT_STATICSTIC == 1)
	uint32_t volatile dma_buffer_not_ready_h;
	uint32_t volatile dma_buffer_not_ready_l;
	int volatile real_data_come;
#endif
#endif
};


enum yiis_dma_direction {
	YIIS_DMA_DIRECTION_UNKNOWN = -1,
	YIIS_DMA_DIRECTION_RX,
	YIIS_DMA_DIRECTION_TX,
	YIIS_DMA_DIRECTION_MAX
};


int yiis_init(struct yiis_ctrl *iis);
int yiis_deinit(struct yiis_ctrl *iis);

int yiis_config(struct yiis_ctrl *iis, enum yiis_dma_direction dir,
				uint32_t sampling_rate, uint8_t channels, uint8_t bit_depth,
				enum IIS_AUDIO_STANDARD audio_standard);

int yiis_dma_start(struct yiis_ctrl *iis, enum yiis_dma_direction dir,
					struct YRingBuffer *rb, uint16_t data_length);
int yiis_dma_end(struct yiis_ctrl *iis, enum yiis_dma_direction dir);

void yiis_dma_disable_interrupts(struct yiis_ctrl *iis, enum yiis_dma_direction dir);
void yiis_dma_enable_interrupts(struct yiis_ctrl *iis, enum yiis_dma_direction dir);

void yiis_dma_pause(struct yiis_ctrl *iis, enum yiis_dma_direction dir);
void yiis_dma_restore(struct yiis_ctrl *iis, enum yiis_dma_direction dir);

int32_t yiis_transfer_data(struct yiis_ctrl *iis, uint8_t *data, uint16_t data_length);
int32_t yiis_receive_data(struct yiis_ctrl *iis, uint8_t *buf, uint16_t length);


extern struct yiis_ctrl *YIIS_2_CTRL;
extern struct yiis_ctrl *YIIS_3_CTRL;


#if (DEFAULT_IIS_OUTPUT_DBG_MSG == 0)
#define YIIS_DBG(...)
#else
#include "../../lib/cmdline/basic_io.h"
#define YIIS_DBG(...)			basic_io_printf("[YIIS]" __VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif
#endif
