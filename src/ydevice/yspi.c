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
#include "../yos/yos.h"
#include "yspi.h"

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
#include "../yos/ymutex.h"
#endif

static struct yspi_device *volatile _selected_slave = NULL;

static void _yspi_1_gpio_init(int is_init)
{
	if (is_init) {
		/* Init gpio for SPI */
		rcc_periph_clock_enable(DEFAULT_SPI_GPIO_RCC);
		gpio_mode_setup(DEFAULT_SPI_GPIO_PORT, GPIO_MODE_AF,
						GPIO_PUPD_NONE, DEFAULT_SPI_GPIO_SCK | DEFAULT_SPI_GPIO_MOSI | DEFAULT_SPI_GPIO_MISO);

		gpio_set_output_options(DEFAULT_SPI_GPIO_PORT, GPIO_OTYPE_PP,
								GPIO_OSPEED_50MHZ, DEFAULT_SPI_GPIO_SCK | DEFAULT_SPI_GPIO_MOSI);

		gpio_set_af(DEFAULT_SPI_GPIO_PORT, GPIO_AF5,
					DEFAULT_SPI_GPIO_SCK | DEFAULT_SPI_GPIO_MOSI | DEFAULT_SPI_GPIO_MISO);
	} else {
		/* Deinit gpio of SPI */
		rcc_periph_clock_disable(DEFAULT_SPI_GPIO_RCC);
	}
}

static struct yspi_ctrl _yspi_1_ctrl = {
	.spi_rcc = RCC_SPI1,
	.spi_base = SPI1,
	.spi_nvic_irq = NVIC_SPI1_IRQ,
	.spi_baudrate = SPI_CR1_BAUDRATE_FPCLK_DIV_2,
	.spi_gpio_init = _yspi_1_gpio_init,
	.spi_cpol = DEFAULT_SPI_CPOL,
	.spi_cpha = DEFAULT_SPI_CPHA,
	.spi_data_format = DEFAULT_SPI_DATA_FMT,
	.spi_msb_lsb_first = DEFAULT_SPI_MSB_LSB_FIRST,

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
//	struct ymutex spi_mutex;
#endif

#if (DEFAULT_SPI_USE_DMA == 1)
	.spi_dma_rx_rcc = RCC_DMA2,
	.spi_dma_rx_base = DMA2,
	.spi_dma_rx_stream = DMA_STREAM0,
	.spi_dma_rx_chsel = DMA_SxCR_CHSEL_3,
	.spi_dma_rx_mem_size = DMA_SxCR_MSIZE_8BIT,
	.spi_dma_rx_peri_size = DMA_SxCR_PSIZE_8BIT,
	.spi_dma_rx_peri_addr = (uint32_t)(&SPI1_DR),
	.spi_dma_rx_nvic_irq = NVIC_DMA2_STREAM0_IRQ,

	.spi_dma_tx_rcc = RCC_DMA2,
	.spi_dma_tx_base = DMA2,
	.spi_dma_tx_stream = DMA_STREAM5,
	.spi_dma_tx_chsel = DMA_SxCR_CHSEL_3,
	.spi_dma_tx_mem_size = DMA_SxCR_MSIZE_8BIT,
	.spi_dma_tx_peri_size = DMA_SxCR_PSIZE_8BIT,
	.spi_dma_tx_peri_addr = (uint32_t)(&SPI1_DR),
	.spi_dma_tx_nvic_irq = NVIC_DMA2_STREAM5_IRQ,

	.spi_dma_is_recving = 0,
	.spi_dma_recv_half = 0,
	.spi_dma_recv_error = 0,

	.spi_dma_is_sending = 0,
	.spi_dma_send_half = 0,
	.spi_dma_send_error = 0

	//uint8_t spi_dma_rx_buffer[DEFAULT_SPI_DMA_RX_BUFFER_SIZE];
	//uint8_t spi_dma_tx_buffer[DEFAULT_SPI_DMA_TX_BUFFER_SIZE];
#endif
};

struct yspi_ctrl *YSPI_1_CTRL = &_yspi_1_ctrl;


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

#if (DEFAULT_SPI_USE_DMA == 1)
static void _yspi_start_rx_dma(struct yspi_ctrl *spi, uint32_t transfer_size)
{
	if (spi == NULL) {
		goto para_err;
	}

	spi->spi_dma_is_recving = 1;
	spi->spi_dma_recv_half = 0;
	spi->spi_dma_recv_error = 0;
	dma_stream_reset(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_set_peripheral_address(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, spi->spi_dma_rx_peri_addr);
	dma_set_memory_address(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, (uint32_t)spi->spi_dma_rx_buffer);
	dma_set_number_of_data(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, transfer_size);
	dma_channel_select(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, spi->spi_dma_rx_chsel);
	dma_set_dma_flow_control(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	//dma_set_priority(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, DMA_SxCR_PL_MEDIUM);
	dma_enable_direct_mode(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_set_transfer_mode(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
	dma_enable_memory_increment_mode(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_disable_peripheral_increment_mode(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_set_memory_size(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, spi->spi_dma_rx_mem_size);
	dma_set_peripheral_size(spi->spi_dma_rx_base, spi->spi_dma_rx_stream, spi->spi_dma_rx_peri_size);
	dma_enable_transfer_complete_interrupt(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_enable_half_transfer_interrupt(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_enable_transfer_error_interrupt(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_enable_stream(spi->spi_dma_rx_base ,spi->spi_dma_rx_stream);
	spi_enable_rx_dma(spi->spi_base);

para_err:
	return;
}

static int _yspi_wait_rx_dma_done(struct yspi_ctrl *spi)
{
	int ret = -1;
	YSPI_DBG("Enter _yspi_wait_rx_dma_done(spi=0x%08X)\n", spi);
	if (spi == NULL) {
		goto para_err;
	}

	while (1) {
		if (spi->spi_dma_is_recving == 0) {
			ret = 0;
			YSPI_DBG("  _dma_is_recving=0\n");
			break;
		}
		if (spi->spi_dma_recv_half == 1) {
			YSPI_DBG("  _dma_recv_half=1\n");
			spi->spi_dma_recv_half = 0;
		}
		if (spi->spi_dma_recv_error == 1) {
			YSPI_DBG("  _dma_recv_error=1\n");
			break;
		}
		yos_task_delay(1);
	}
	YSPI_DBG("  dma recving done\n");
	spi_disable_rx_dma(spi->spi_base);
	dma_disable_stream(spi->spi_dma_rx_base ,spi->spi_dma_rx_stream);
	YSPI_DBG("Leave _yspi_wait_rx_dma_done, ret=%d\n", ret);

para_err:
	return ret;
}

static void _yspi_start_tx_dma(struct yspi_ctrl *spi, uint32_t transfer_size)
{
	if (spi == NULL) {
		goto para_err;
	}

	spi->spi_dma_is_sending = 1;
	spi->spi_dma_send_half = 0;
	spi->spi_dma_send_error = 0;
	dma_stream_reset(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_set_peripheral_address(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, spi->spi_dma_tx_peri_addr);
	dma_set_memory_address(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, (uint32_t)spi->spi_dma_tx_buffer);
	dma_set_number_of_data(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, transfer_size);
	dma_channel_select(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, spi->spi_dma_tx_chsel);
	dma_set_dma_flow_control(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	//dma_set_priority(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, DMA_SxCR_PL_MEDIUM);
	dma_enable_direct_mode(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_set_transfer_mode(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_enable_memory_increment_mode(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_disable_peripheral_increment_mode(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_set_memory_size(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, spi->spi_dma_tx_mem_size);
	dma_set_peripheral_size(spi->spi_dma_tx_base, spi->spi_dma_tx_stream, spi->spi_dma_tx_peri_size);
	dma_enable_transfer_complete_interrupt(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_enable_half_transfer_interrupt(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_enable_transfer_error_interrupt(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	dma_enable_stream(spi->spi_dma_tx_base ,spi->spi_dma_tx_stream);
	spi_enable_tx_dma(spi->spi_base);

para_err:
	return;
}

static int _yspi_wait_tx_dma_done(struct yspi_ctrl *spi)
{
	int ret = -1;
	YSPI_DBG("Enter _yspi_wait_tx_dma_done(spi=0x%08X)\n", spi);
	if (spi == NULL) {
		goto para_err;
	}

	while (1) {
		if (spi->spi_dma_is_sending == 0) {
			ret = 0;
			YSPI_DBG("  _dma_is_sending=0\n");
			break;
		}
		if (spi->spi_dma_send_half == 1) {
			YSPI_DBG("  _dma_send_half=1\n");
			spi->spi_dma_send_half = 0;
		}
		if (spi->spi_dma_send_error == 1) {
			YSPI_DBG("  _dma_send_error=1\n");
			break;
		}
		yos_task_delay(1);
	}
	YSPI_DBG("  dma sending done\n");
	while (!(SPI_SR(spi->spi_base) & SPI_SR_TXE)) {
		yos_task_delay(1);
	}
	YSPI_DBG("  TXE=1\n");
	while ((SPI_SR(spi->spi_base) & SPI_SR_BSY)) {
		yos_task_delay(1);
	}
	YSPI_DBG("  BSY=0\n");
	spi_disable_tx_dma(spi->spi_base);
	dma_disable_stream(spi->spi_dma_tx_base ,spi->spi_dma_tx_stream);
	YSPI_DBG("Leave _yspi_wait_tx_dma_done, ret=%d\n", ret);

para_err:
	return ret;
}
#endif


int yspi_master_init(struct yspi_ctrl *spi)
{
	int ret = -1;
	if (spi == NULL) {
		goto para_err;
	}

	rcc_periph_clock_enable(spi->spi_rcc);

#if (DEFAULT_SPI_USE_DMA == 1)
	rcc_periph_clock_enable(spi->spi_dma_rx_rcc);
	rcc_periph_clock_enable(spi->spi_dma_tx_rcc);

	/* Set RX DMA */
	//nvic_set_priority(DEFAULT_SPI_DMA_RX_NVIC_IRQ, 0);
	nvic_enable_irq(spi->spi_dma_rx_nvic_irq);
	//dma_disable_stream(DEFAULT_SPI_DMA_RX ,DEFAULT_SPI_DMA_RX_STREAM);

	/* Set TX DMA */
	//nvic_set_priority(DEFAULT_SPI_DMA_TX_NVIC_IRQ, 0);
	nvic_enable_irq(spi->spi_dma_tx_nvic_irq);
	//dma_disable_stream(DEFAULT_SPI_DMA_TX ,DEFAULT_SPI_DMA_TX_STREAM);

	spi->spi_dma_is_sending = 0;
	spi->spi_dma_is_recving = 0;
#endif

	if (spi->spi_gpio_init != NULL) {
		spi->spi_gpio_init(1);
	}

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_init(&spi->spi_mutex);
#endif

	spi_disable(spi->spi_base);

	if (spi_init_master(spi->spi_base, spi->spi_baudrate, spi->spi_cpol, spi->spi_cpha,
					spi->spi_data_format, spi->spi_msb_lsb_first) != 0) {
		goto master_init_err;
	}

	spi_set_full_duplex_mode(spi->spi_base);
	spi_enable_software_slave_management(spi->spi_base);
	spi_set_nss_high(spi->spi_base);

	//nvic_enable_irq(DEFAULT_SPI_NVIC_IRQ);
	//spi_enable_rx_buffer_not_empty_interrupt(DEFAULT_SPI);
	//spi_enable_tx_buffer_empty_interrupt(DEFAULT_SPI);
	spi_enable(spi->spi_base);

	ret = 0;

	return ret;

master_init_err:
	yspi_master_deinit(spi);
para_err:
	return ret;
}

int yspi_master_deinit(struct yspi_ctrl *spi)
{
	int ret = -1;
	if (spi == NULL) {
		goto para_err;
	}
#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_deinit(&spi->spi_mutex);
#endif

	//nvic_disable_irq(DEFAULT_SPI_NVIC_IRQ);

#if (DEFAULT_SPI_USE_DMA == 1)
	spi_disable_rx_dma(spi->spi_base);
	spi_disable_tx_dma(spi->spi_base);
	dma_disable_stream(spi->spi_dma_rx_base, spi->spi_dma_rx_stream);
	dma_disable_stream(spi->spi_dma_tx_base, spi->spi_dma_tx_stream);
	nvic_disable_irq(spi->spi_dma_rx_nvic_irq);
	nvic_disable_irq(spi->spi_dma_tx_nvic_irq);
	rcc_periph_clock_disable(spi->spi_dma_rx_rcc);
	rcc_periph_clock_disable(spi->spi_dma_tx_rcc);
#endif

	spi_disable(spi->spi_base);

	rcc_periph_clock_disable(spi->spi_rcc);

	if (spi->spi_gpio_init != NULL) {
		spi->spi_gpio_init(0);
	}

	return 0;

para_err:
	return ret;
}

int yspi_master_set_speed(struct yspi_ctrl *spi, uint32_t freq)
{
	int ret = -1;
	uint8_t bd;

	if (spi == NULL) {
		goto para_err;
	}

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
	spi_set_baudrate_prescaler(spi->spi_base, bd);

	ret = 0;

para_err:
	return ret;
}

int yspi_trans_begin(struct yspi_ctrl *spi, struct yspi_device *cs)
{
	int ret = -1;
	if (spi == NULL || cs == NULL) {
		goto para_err;
	}

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_lock(&spi->spi_mutex);
#endif

	if (cs->is_in_transaction) {
		goto tran_err;
	}

	if (yspi_device_select(cs) != 0) {
		goto cs_err;
	}

	cs->is_in_transaction = 1;
	_selected_slave = cs;

	ret = 0;

	return ret;

cs_err:
tran_err:
#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	ymutex_unlock(&spi->spi_mutex);
#endif
para_err:
	return ret;
}

int yspi_trans_end(struct yspi_ctrl *spi, struct yspi_device *cs)
{
	int ret = -1;
	if (spi == NULL || cs == NULL) {
		goto para_err;
	}

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	if (ymutex_try_lock(&spi->spi_mutex) == 0) {
		ymutex_unlock(&spi->spi_mutex);
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
	_selected_slave = NULL;

#if (DEFAULT_SPI_TRAN_WITH_MUTEX == 1)
	if (ymutex_unlock(&spi->spi_mutex) == 0) {
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

static uint32_t _yspi_send_once(struct yspi_ctrl *spi, void *data, uint32_t data_len)
{
	uint32_t bytes_sent = 0;
	if (spi == NULL || data == NULL || data_len == 0) {
		goto para_err;
	}

#if (DEFAULT_SPI_USE_DMA == 1)
	uint32_t send_size;
	if (data_len > sizeof(spi->spi_dma_tx_buffer)) {
		send_size = sizeof(spi->spi_dma_tx_buffer);
		YSPI_DBG("yspi_send: data len[%d] exceeds buffer size[%d]\n",
				data_len, sizeof(spi->spi_dma_tx_buffer));
	} else {
		send_size = data_len;
	}

	memcpy(spi->spi_dma_tx_buffer, data, send_size);
	_yspi_start_tx_dma(spi, send_size);
	if (_yspi_wait_tx_dma_done(spi) == 0) {
		bytes_sent = send_size;
	}
#else
	while (bytes_sent < data_len) {
		spi_send(spi->spi_base, (uint16_t)(*((uint8_t *)data + bytes_sent)));
		bytes_sent++;
	}
#endif

para_err:
	return bytes_sent;
}

uint32_t yspi_send(struct yspi_ctrl *spi, void *data, uint32_t data_len)
{
	YSPI_DBG("Enter yspi_send(spi=0x%08X, data=0x%08X, len=%d)\n", spi, data, data_len);
	uint32_t bytes_sent = 0;
	uint32_t byte_sent_once;
	if (spi == NULL || data == NULL || data_len == 0) {
		goto para_err;
	}

	while (bytes_sent < data_len) {
		byte_sent_once = _yspi_send_once(spi,
										(uint8_t *)data + bytes_sent,
										data_len - bytes_sent);
		if (byte_sent_once > 0) {
			bytes_sent += byte_sent_once;
		} else {
			goto send_err;
		}
	}

send_err:
para_err:
	YSPI_DBG("Leave yspi_send ret=%d\n", bytes_sent);
	return bytes_sent;
}

static uint32_t _yspi_receive_once(struct yspi_ctrl *spi, void *buf, uint32_t buf_len)
{
	uint32_t bytes_received = 0;
	if (spi == NULL || buf == NULL || buf_len == 0) {
		goto para_err;
	}

#if (DEFAULT_SPI_USE_DMA == 1)
	uint32_t recv_size;
	if (buf_len > sizeof(spi->spi_dma_rx_buffer)) {
		recv_size = sizeof(spi->spi_dma_rx_buffer);
		YSPI_DBG("yspi_receive: data len[%d] exceeds buffer size[%d]\n",
				buf_len, sizeof(spi->spi_dma_rx_buffer));
	} else {
		recv_size = buf_len;
	}

	_yspi_start_rx_dma(spi, recv_size);
	if (_yspi_wait_rx_dma_done(spi) == 0) {
		memcpy(buf, spi->spi_dma_rx_buffer, recv_size);
		bytes_received = recv_size;
	}
#else
	while (bytes_received < buf_len) {
		*((uint8_t *)buf + bytes_received) = (uint8_t)spi_read(spi->spi_base);
		bytes_received++;
	}
#endif

para_err:
	return bytes_received;
}

uint32_t yspi_receive(struct yspi_ctrl *spi, void *buf, uint32_t buf_len)
{
	YSPI_DBG("Enter yspi_receive(spi=0x%08X, buf=0x%08X, len=%d)\n", spi, buf, buf_len);
	uint32_t bytes_recved = 0;
	uint32_t byte_recv_once;
	if (spi == NULL || buf == NULL || buf_len == 0) {
		goto para_err;
	}

	while (bytes_recved < buf_len) {
		byte_recv_once = _yspi_receive_once(spi,
											(uint8_t *)buf + bytes_recved,
											buf_len - bytes_recved);
		if (byte_recv_once > 0) {
			bytes_recved += byte_recv_once;
		} else {
			goto recv_err;
		}
	}

recv_err:
para_err:
	YSPI_DBG("Leave yspi_receive, ret=%d\n", bytes_recved);
	return bytes_recved;
}

static uint32_t _yspi_send_and_receive_once(struct yspi_ctrl *spi,
											void *send_data,
											void *recv_buf,
											uint32_t data_len,
											uint8_t send_byte_filler)
{
	uint32_t bytes_transfered = 0;
	YSPI_DBG("Enter _yspi_send_and_receive_once(spi=0x%08X, send_data=0x%08X, recv_buf=0x%08X, length=%d, fill byte=0x%02X)\n",
			spi, send_data, recv_buf, data_len, send_byte_filler);
	if (spi == NULL || (send_data == NULL && recv_buf == NULL) || data_len == 0) {
		goto para_err;
	}

#if (DEFAULT_SPI_USE_DMA == 1)
	uint32_t real_bytes;
	if (sizeof(spi->spi_dma_tx_buffer) != sizeof(spi->spi_dma_rx_buffer)) {
		goto buffer_err;
	}
	if (data_len > sizeof(spi->spi_dma_tx_buffer)) {
		real_bytes = sizeof(spi->spi_dma_tx_buffer);
		YSPI_DBG("yspi_send_and_receive: data len[%d] exceeds buffer size[%d]\n",
				data_len, sizeof(spi->spi_dma_tx_buffer));
	} else {
		real_bytes = data_len;
	}

	if (send_data != NULL) {
		memcpy(spi->spi_dma_tx_buffer, send_data, real_bytes);
	} else {
		memset(spi->spi_dma_tx_buffer, send_byte_filler, real_bytes);
	}

	_yspi_start_rx_dma(spi, real_bytes);
	_yspi_start_tx_dma(spi, real_bytes);
	if (_yspi_wait_rx_dma_done(spi) == 0 && _yspi_wait_tx_dma_done(spi) == 0) {
		if (recv_buf != NULL) {
			memcpy(recv_buf, spi->spi_dma_rx_buffer, real_bytes);
		}

		bytes_transfered = real_bytes;
	}
#else
	uint8_t send_byte, recv_byte;
	for (bytes_transfered = 0; bytes_transfered < data_len; bytes_transfered++) {
		if (send_data == NULL) {
			send_byte = send_byte_filler;
		} else {
			send_byte = *((uint8_t *)send_data + bytes_transfered);
		}
		recv_byte = yspi_write_and_read_byte(spi, send_byte);
		if (recv_buf != NULL) {
			*((uint8_t *)recv_buf + bytes_transfered) = recv_byte;
		}
	}
#endif

#if (DEFAULT_SPI_USE_DMA == 1)
buffer_err:
#endif
para_err:
	YSPI_DBG("Leave _yspi_send_and_receive_once, bytes transfered=%d\n", bytes_transfered);

	return bytes_transfered;
}


uint32_t yspi_send_and_receive(struct yspi_ctrl *spi,
							void *send_data,
							void *recv_buf,
							uint32_t data_len,
							uint8_t send_byte_filler)
{
	uint32_t bytes_xfered = 0;
	uint32_t bytes_xfered_once;
	if (spi == NULL || (send_data == NULL && recv_buf == NULL)) {
		goto para_err;
	}
	while (bytes_xfered < data_len) {
		bytes_xfered_once = _yspi_send_and_receive_once(spi, 
														(uint8_t *)send_data + bytes_xfered,
														(uint8_t *)recv_buf + bytes_xfered,
														data_len - bytes_xfered,
														send_byte_filler);
		if (bytes_xfered_once > 0) {
			bytes_xfered += bytes_xfered_once;
		} else {
			goto transfer_err;
		}
	}

transfer_err:
para_err:
	return bytes_xfered;
}


uint8_t yspi_write_and_read_byte(struct yspi_ctrl *spi, uint8_t data)
{
	uint8_t ret = 0xFF;
	if (spi == NULL) {
		goto para_err;
	}

	ret = spi_xfer(spi->spi_base, data);

para_err:
	return ret;
}

uint8_t yspi_trans_write_and_read_byte(struct yspi_ctrl *spi,
									struct yspi_device *cs,
									uint8_t data)
{
	uint8_t d = 0xFF;
	if (spi == NULL || cs == NULL) {
		goto para_err;
	}

	if (yspi_trans_begin(spi, cs) != 0) {
		goto tran_err;
	}

	d = yspi_write_and_read_byte(spi, data);

	yspi_trans_end(spi, cs);

tran_err:
para_err:
	return d;
}


void DEFAULT_SPI_ISR_FUNC(void)
{
}

#if (DEFAULT_SPI_USE_DMA == 1)
/* SPI1_DMA_RX isr */
void dma2_stream0_isr(void)
{
	if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_TCIF)) {
		/* Transfer complete */
		dma_disable_transfer_complete_interrupt(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream);
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_TCIF);

		YSPI_1_CTRL->spi_dma_is_recving = 0;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_RX_COMPLETE);
		}
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_HTIF)) {
		/* Half transferred */
		dma_disable_half_transfer_interrupt(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream);
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_HTIF);

		YSPI_1_CTRL->spi_dma_recv_half = 1;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_RX_HALF_TRANSFERED);
		}
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_TEIF)) {
		/* Transfer error */
		dma_disable_transfer_error_interrupt(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream);
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_TEIF);

		YSPI_1_CTRL->spi_dma_recv_error = 1;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_RX_ERROR);
		}
#if 0
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_DMEIF);
		dma_disable_stream(YSPI_1_CTRL->spi_dma_rx_base ,YSPI_1_CTRL->spi_dma_rx_stream);

		YSPI_1_CTRL->spi_dma_is_recving = 0;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_RX_DIRECT_ERROR);
		}
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_rx_base, YSPI_1_CTRL->spi_dma_rx_stream, DMA_FEIF);
		dma_disable_stream(YSPI_1_CTRL->spi_dma_rx_base ,YSPI_1_CTRL->spi_dma_rx_stream);

		YSPI_1_CTRL->spi_dma_is_recving = 0;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_RX_FIFO_ERROR);
		}
	} else {
#endif
	}
}

/* SPI1_DMA_TX isr */
void dma2_stream5_isr(void)
{
	if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_TCIF)) {
		/* Transfer complete */
		dma_disable_transfer_complete_interrupt(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream);
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_TCIF);

		YSPI_1_CTRL->spi_dma_is_sending = 0;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_TX_COMPLETE);
		}
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_HTIF)) {
		/* Half transferred */
		dma_disable_half_transfer_interrupt(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream);
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_HTIF);

		YSPI_1_CTRL->spi_dma_send_half = 1;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_TX_HALF_TRANSFERED);
		}
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_TEIF)) {
		/* Transfer error */
		dma_disable_transfer_error_interrupt(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream);
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_TEIF);

		YSPI_1_CTRL->spi_dma_send_error = 1;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_TX_ERROR);
		}
#if 0
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_DMEIF);
		dma_disable_stream(YSPI_1_CTRL->spi_dma_tx_base ,YSPI_1_CTRL->spi_dma_tx_stream);

		YSPI_1_CTRL->spi_dma_is_sending = 0;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_TX_DIRECT_ERROR);
		}
	} else if (dma_get_interrupt_flag(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(YSPI_1_CTRL->spi_dma_tx_base, YSPI_1_CTRL->spi_dma_tx_stream, DMA_FEIF);
		dma_disable_stream(YSPI_1_CTRL->spi_dma_tx_base ,YSPI_1_CTRL->spi_dma_tx_stream);

		YSPI_1_CTRL->spi_dma_is_sending = 0;
		if (_selected_slave != NULL && _selected_slave->on_event != NULL) {
			_selected_slave->on_event(YSPI_DEVICE_EVENT_TX_FIFO_ERROR);
		}
	} else {
#endif
	}
}
#endif
