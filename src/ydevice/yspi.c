/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include <stdio.h>
#include <string.h>
#include "yspi.h"

#define DEFAULT_SPI_USE_DMA				0

#define DEFAULT_SPI_TRAN_WITH_MUTEX		0

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
#include "../yos/ymutex.h"
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

#if (DEFAULT_SPI_USE_DMA == 1)
/* SPI DMA Settings: RX */
#define DEFAULT_SPI_DMA_RX				DMA2
#define DEFAULT_SPI_DMA_RX_STREAM		DMA_STREAM2
#define DEFAULT_SPI_DMA_RX_CHANNEL		DMA_SxCR_CHSEL_3
#define DEFAULT_SPI_DMA_RX_PERI_ADDR	SPI1_DR
#define DEFAULT_SPI_DMA_RX_ISR			dma2_stream2_isr

/* SPI DMA Settings: TX */
#define DEFAULT_SPI_DMA_TX				DMA2
#define DEFAULT_SPI_DMA_TX_STREAM		DMA_STREAM3
#define DEFAULT_SPI_DMA_TX_CHANNEL		DMA_SxCR_CHSEL_3
#define DEFAULT_SPI_DMA_TX_PERI_ADDR	SPI1_DR
#define DEFAULT_SPI_DMA_TX_ISR			dma2_stream3_isr
#endif

/* SPI Settings */
#define DEFAULT_SPI_BAUDRATE			SPI_CR1_BAUDRATE_FPCLK_DIV_2
#define DEFAULT_SPI_CPOL				SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE
#define DEFAULT_SPI_CPHA				SPI_CR1_CPHA_CLK_TRANSITION_1
#define DEFAULT_SPI_DATA_FMT			SPI_CR1_DFF_8BIT
#define DEFAULT_SPI_MSB_LSB_FIRST		SPI_CR1_MSBFIRST

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
/* SPI Mutex */
static struct ymutex _yspi_mutex;
#endif

int yspi_device_init(struct yspi_device *dev, uint32_t gpio_port,
					uint16_t gpio_num, enum YSPI_CS_VALID_VALUE valid_value,
					void (*on_event)(enum YSPI_DEVICE_EVENT evt))
{
	int ret = -1;
	uint8_t pull_up_or_down;

	if (dev == NULL) {
		goto para_err;
	}

	memset(dev, 0x00, sizeof(*dev));

	if (valid_value == YSPI_CS_VALID_ON_LOW) {
		pull_up_or_down = GPIO_PUPD_PULLUP;
	} else if (valid_value == YSPI_CS_VALID_ON_HIGH) {
		pull_up_or_down = GPIO_PUPD_PULLDOWN;
	} else {
		goto para_err;
	}

	gpio_mode_setup(gpio_port, GPIO_MODE_OUTPUT, pull_up_or_down, gpio_num);
	gpio_set_output_options(gpio_port, GPIO_OTYPE_PP,
							GPIO_OSPEED_50MHZ, gpio_num);

	dev->cs_gpio_port = gpio_port;
	dev->cs_gpio_num = gpio_num;
	dev->valid_value = valid_value;
	dev->is_in_transaction = 0;
	dev->on_event = on_event;

	yspi_device_unselect(dev);

	ret = 0;

para_err:
	return ret;
}

int yspi_device_deinit(struct yspi_device *dev)
{
	int ret = -1;
	if (dev == NULL) {
		goto para_err;
	}

	//yspi_device_unselect(dev);
	//dev->is_in_transaction = 0;

	gpio_mode_setup(dev->cs_gpio_port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, dev->cs_gpio_num);
	memset(dev, 0x00, sizeof(*dev));

	ret = 0;

para_err:
	return ret;
}

int yspi_device_select(struct yspi_device *dev)
{
	int ret = -1;
	if (dev == NULL) {
		goto para_err;
	}

	if (dev->valid_value == YSPI_CS_VALID_ON_LOW) {
		gpio_clear(dev->cs_gpio_port, dev->cs_gpio_num);
		ret = 0;
	} else if (dev->valid_value == YSPI_CS_VALID_ON_HIGH) {
		gpio_set(dev->cs_gpio_port, dev->cs_gpio_num);
		ret = 0;
	} else {
		goto cs_err;
	}

cs_err:
para_err:
	return ret;
}

int yspi_device_unselect(struct yspi_device *dev)
{
	int ret = -1;
	if (dev == NULL) {
		goto para_err;
	}

	if (dev->valid_value == YSPI_CS_VALID_ON_LOW) {
		gpio_set(dev->cs_gpio_port, dev->cs_gpio_num);
		ret = 0;
	} else if (dev->valid_value == YSPI_CS_VALID_ON_HIGH) {
		gpio_clear(dev->cs_gpio_port, dev->cs_gpio_num);
		ret = 0;
	} else {
		goto cs_err;
	}

cs_err:
para_err:
	return ret;
}


int yspi_master_init(void)
{
	int ret = -1;

	rcc_periph_clock_enable(DEFAULT_SPI_RCC);
	rcc_periph_clock_enable(DEFAULT_SPI_GPIO_RCC);

	gpio_mode_setup(DEFAULT_SPI_GPIO_PORT, GPIO_MODE_AF,
					GPIO_PUPD_NONE, DEFAULT_SPI_GPIO_SCK | DEFAULT_SPI_GPIO_MOSI | DEFAULT_SPI_GPIO_MISO);

	gpio_set_output_options(DEFAULT_SPI_GPIO_PORT, GPIO_OTYPE_PP,
							GPIO_OSPEED_50MHZ, DEFAULT_SPI_GPIO_SCK | DEFAULT_SPI_GPIO_MOSI);

	gpio_set_af(DEFAULT_SPI_GPIO_PORT, GPIO_AF5,
				DEFAULT_SPI_GPIO_SCK | DEFAULT_SPI_GPIO_MOSI | DEFAULT_SPI_GPIO_MISO);

	//spi_enable(DEFAULT_SPI);
	//nvic_enable_irq(DEFAULT_SPI_NVIC_IRQ);

#if (DEFAULT_SPI_USE_DMA == 1)
	spi_enable_rx_dma(DEFAULT_SPI);
	dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);

	spi_enable_tx_dma(DEFAULT_SPI);
	dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);
#endif

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_init(&_yspi_mutex);
#endif

	spi_disable(DEFAULT_SPI);

	if (spi_init_master(DEFAULT_SPI, DEFAULT_SPI_BAUDRATE, DEFAULT_SPI_CPOL, DEFAULT_SPI_CPHA,
					DEFAULT_SPI_DATA_FMT, DEFAULT_SPI_MSB_LSB_FIRST) != 0) {
		goto master_init_err;
	}

	spi_set_full_duplex_mode(DEFAULT_SPI);
	spi_enable_software_slave_management(DEFAULT_SPI);
	spi_set_nss_high(DEFAULT_SPI);

	nvic_enable_irq(DEFAULT_SPI_NVIC_IRQ);
	spi_enable(DEFAULT_SPI);

	ret = 0;

master_init_err:
	return ret;
}

int yspi_master_deinit(void)
{
#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_deinit(&_yspi_mutex);
#endif

	nvic_disable_irq(DEFAULT_SPI_NVIC_IRQ);

#if (DEFAULT_SPI_USE_DMA == 1)
	spi_disable_rx_dma(DEFAULT_SPI);
	spi_disable_tx_dma(DEFAULT_SPI);
#endif

	spi_disable(DEFAULT_SPI);

	rcc_periph_clock_disable(DEFAULT_SPI_RCC);
	rcc_periph_clock_disable(DEFAULT_SPI_GPIO_RCC);

	return 0;
}

int yspi_master_set_speed(uint32_t freq)
{
	uint8_t bd;
	if (freq >= DEFAULT_FPCLK / 2) {
		bd = SPI_CR1_BR_FPCLK_DIV_2;
	} else if (freq >= DEFAULT_FPCLK / 4) {
		bd = SPI_CR1_BR_FPCLK_DIV_4;
	} else if (freq >= DEFAULT_FPCLK / 8) {
		bd = SPI_CR1_BR_FPCLK_DIV_8;
	} else if (freq >= DEFAULT_FPCLK / 16) {
		bd = SPI_CR1_BR_FPCLK_DIV_16;
	} else if (freq >= DEFAULT_FPCLK / 32) {
		bd = SPI_CR1_BR_FPCLK_DIV_32;
	} else if (freq >= DEFAULT_FPCLK / 64) {
		bd = SPI_CR1_BR_FPCLK_DIV_64;
	} else if (freq >= DEFAULT_FPCLK / 128) {
		bd = SPI_CR1_BR_FPCLK_DIV_128;
	} else if (freq >= DEFAULT_FPCLK / 256) {
		bd = SPI_CR1_BR_FPCLK_DIV_256;
	} else {
		bd = SPI_CR1_BR_FPCLK_DIV_256;
	}
	spi_set_baudrate_prescaler(DEFAULT_SPI, bd);

	return 0;
}

static struct yspi_device *volatile current_device = NULL;
int yspi_trans_begin(struct yspi_device *cs)
{
	int ret = -1;
	if (cs == NULL) {
		goto para_err;
	}

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_lock(&_yspi_mutex);
#endif

	if (cs->is_in_transaction) {
		goto tran_err;
	}

	if (yspi_device_select(cs) != 0) {
		goto cs_err;
	}

	cs->is_in_transaction = 1;
	current_device = cs;

	ret = 0;

	return ret;

cs_err:
tran_err:
#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_unlock(&_yspi_mutex);
#endif
para_err:
	return ret;
}

int yspi_trans_end(struct yspi_device *cs)
{
	int ret = -1;
	if (cs == NULL) {
		goto para_err;
	}

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	if (ymutex_try_lock(&_yspi_mutex) == 0) {
		ymutex_unlock(&_yspi_mutex);
		goto mutex_err;
	}
#endif

	if (!cs->is_in_transaction) {
		goto tran_err;
	}

	if (yspi_device_unselect(cs) != 0) {
		goto cs_err;
	}

	cs->is_in_transaction = 0;
	current_device = NULL;

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	if (ymutex_unlock(&_yspi_mutex) == 0) {
		ret = 0;
	}
#else
	ret = 0;
#endif

cs_err:
tran_err:
#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
mutex_err:
#endif
para_err:
	return ret;
}

uint32_t yspi_trans_send(void *data, uint32_t data_len)
{
	uint32_t bytes_sent = 0;
	if (data == NULL || data_len == 0) {
		goto para_err;
	}

#if (DEFAULT_SPI_USE_DMA == 1)
	dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);
	dma_channel_select(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DEFAULT_SPI_DMA_TX_CHANNEL);
	dma_set_transfer_mode(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_set_dma_flow_control(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM);
	dma_set_memory_address(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, data);
	dma_set_memory_size(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_SxCR_MSIZE_8BIT);
	dma_set_number_of_data(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, data_len);
	dma_enable_memory_increment_mode(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM);
	dma_set_peripheral_address(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DEFAULT_SPI_DMA_TX_PERI_ADDR);
	dma_enable_fifo_mode(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM);
	dma_enable_transfer_complete_interrupt(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM);

	dma_enable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);
#else
	while (bytes_sent < data_len) {
		spi_send(DEFAULT_SPI, (uint16_t)(*((uint8_t *)data + bytes_sent)));
		bytes_sent++;
	}
#endif

para_err:
	return bytes_sent;
}

uint32_t yspi_trans_receive(void *buf, uint32_t buf_len)
{
	uint32_t bytes_received = 0;
	if (buf == NULL || buf_len == 0) {
		goto para_err;
	}

#if (DEFAULT_SPI_USE_DMA == 1)
	dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);
	dma_channel_select(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DEFAULT_SPI_DMA_RX_CHANNEL);
	dma_set_transfer_mode(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
	dma_set_dma_flow_control(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM);
	dma_set_memory_address(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, buf);
	dma_set_memory_size(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_SxCR_MSIZE_8BIT);
	dma_set_number_of_data(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, buf_len);
	dma_enable_memory_increment_mode(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM);
	dma_set_peripheral_address(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DEFAULT_SPI_DMA_RX_PERI_ADDR);
	dma_enable_fifo_mode(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM);
	dma_enable_transfer_complete_interrupt(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM);

	dma_enable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);
#else
	while (bytes_received < buf_len) {
		*((uint8_t *)buf + bytes_received) = (uint8_t)spi_read(DEFAULT_SPI);
		bytes_received++;
	}
#endif

para_err:
	return bytes_received;
}

uint8_t yspi_trans_write_and_read(struct yspi_device *cs, uint8_t data)
{
	return spi_xfer(DEFAULT_SPI, data);
	//spi_send(DEFAULT_SPI, data);
	//return spi_read(DEFAULT_SPI);

#if 0
	uint8_t d = 0xFF;
	if (yspi_trans_send(&data, sizeof(data)) != sizeof(data)) {
		goto send_err;
	}
	if (yspi_trans_receive(&d, sizeof(d)) != sizeof(d)) {
		goto recv_err;
	}

recv_err:
send_err:
	return d;
#endif
}

uint8_t yspi_write_and_read(struct yspi_device *cs, uint8_t data)
{
	uint8_t d = 0xFF;
	if (yspi_trans_begin(cs) != 0) {
		goto tran_err;
	}

	d = yspi_trans_write_and_read(cs, data);

	yspi_trans_end(cs);

tran_err:
	return d;
}

#if (DEFAULT_SPI_USE_DMA == 1)
void DEFAULT_SPI_DMA_RX_ISR(void)
{
	if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_TCIF)) {
		/* Transfer complete */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_TCIF);
		dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_RX_COMPLETE);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_HTIF)) {
		/* Half transferred */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_HTIF);
		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_RX_HALF_TRANSFERED);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_TEIF)) {
		/* Transfer error */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_TEIF);
		dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_RX_ERROR);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_DMEIF);
		dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_RX_DIRECT_ERROR);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_RX, DEFAULT_SPI_DMA_RX_STREAM, DMA_FEIF);
		dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_RX_FIFO_ERROR);
		}
	} else {
	}
}

void DEFAULT_SPI_DMA_TX_ISR(void)
{
	if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_TCIF)) {
		/* Transfer complete */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_TCIF);
		dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_TX_COMPLETE);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_HTIF)) {
		/* Half transferred */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_HTIF);
		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_TX_HALF_TRANSFERED);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_TEIF)) {
		/* Transfer error */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_TEIF);
		dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_TX_ERROR);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_DMEIF);
		dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_TX_DIRECT_ERROR);
		}
	} else if (dma_get_interrupt_flag(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(DEFAULT_SPI_DMA_TX, DEFAULT_SPI_DMA_TX_STREAM, DMA_FEIF);
		dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);

		if (current_device != NULL && current_device->on_event != NULL) {
			current_device->on_event(YSPI_DEVICE_EVENT_TX_FIFO_ERROR);
		}
	} else {
	}
}
#endif
