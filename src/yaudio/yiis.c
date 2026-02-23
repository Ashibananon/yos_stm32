/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <string.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include "../yos/yos.h"
#include "yiis.h"


enum IIS_CLOCK_TARGET_HZ {
	IIS_CLOCK_TARGET_HZ_8000 = 0,
	IIS_CLOCK_TARGET_HZ_16000,
	IIS_CLOCK_TARGET_HZ_32000,
	IIS_CLOCK_TARGET_HZ_48000,
	IIS_CLOCK_TARGET_HZ_96000,
	IIS_CLOCK_TARGET_HZ_22050,
	IIS_CLOCK_TARGET_HZ_44100,
	IIS_CLOCK_TARGET_HZ_192000,
	IIS_CLOCK_TARGET_HZ_MAX
};

enum IIS_DATA_FORMAT {
	IIS_DATA_FORMAT_16BIT = 0,
	IIS_DATA_FORMAT_32BIT,
	IIS_DATA_FORMAT_MAX
};

static struct IIS_CLOCK_SETTING {
	uint16_t PLLI2SN;
	uint16_t PLLI2SR;
	uint16_t I2SDIV;
	uint16_t I2SODD;
};

const static struct IIS_CLOCK_SETTING STM32F401RC_IIS_CLOCK_SETTINGS[IIS_CLOCK_TARGET_HZ_MAX][IIS_DATA_FORMAT_MAX] = {
	/* IIS_CLOCK_TARGET_HZ_8000 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{192, 2, 187, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{192, 3, 62, 1}
	},
	/* IIS_CLOCK_TARGET_HZ_16000 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{192, 3, 62, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{256, 2, 62, 1}
	},
	/* IIS_CLOCK_TARGET_HZ_32000 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{256, 2, 62, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{256, 5, 12, 1}
	},
	/* IIS_CLOCK_TARGET_HZ_48000 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{192, 5, 12, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{384, 5, 12, 1}
	},
	/* IIS_CLOCK_TARGET_HZ_96000 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{384, 5, 12, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{424, 3, 11, 1}
	},
	/* IIS_CLOCK_TARGET_HZ_22050 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{290, 3, 68, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{302, 2, 53, 1}
	},
	/* IIS_CLOCK_TARGET_HZ_44100 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{302, 2, 53, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{429, 4, 19, 0}
	},
	/* IIS_CLOCK_TARGET_HZ_192000 */
	{
		/* IIS_DATA_FORMAT_16BIT */
		{424, 3, 11, 1},
		/* IIS_DATA_FORMAT_32BIT */
		{258, 3, 3, 1}
	}
};

static void _yiis_2_gpio_init(int is_init)
{
	if (is_init) {
		/* Init gpio for IIS */
		rcc_periph_clock_enable(DEFAULT_IIS_GPIO_RCC);
		gpio_mode_setup(DEFAULT_IIS_GPIO_PORT, GPIO_MODE_AF,
						GPIO_PUPD_NONE, DEFAULT_IIS_GPIO_WS | DEFAULT_IIS_GPIO_CLK | DEFAULT_IIS_GPIO_SD);

		gpio_set_output_options(DEFAULT_IIS_GPIO_PORT, GPIO_OTYPE_PP,
								GPIO_OSPEED_50MHZ, DEFAULT_IIS_GPIO_WS | DEFAULT_IIS_GPIO_CLK | DEFAULT_IIS_GPIO_SD);

		gpio_set_af(DEFAULT_IIS_GPIO_PORT, GPIO_AF5,
					DEFAULT_IIS_GPIO_WS | DEFAULT_IIS_GPIO_CLK | DEFAULT_IIS_GPIO_SD);
	} else {
		/* Deinit gpio of IIS */
		//rcc_periph_clock_disable(DEFAULT_IIS_GPIO_RCC);
	}
}

static void _yiis_3_gpio_init(int is_init)
{
	if (is_init) {
		/* Init gpio for IIS */
		rcc_periph_clock_enable(DEFAULT_IIS_REC_GPIO_RCC_WS);
		rcc_periph_clock_enable(DEFAULT_IIS_REC_GPIO_RCC_CLK_SD);
		gpio_mode_setup(DEFAULT_IIS_REC_GPIO_PORT_WS, GPIO_MODE_AF,
						GPIO_PUPD_NONE, DEFAULT_IIS_REC_GPIO_WS);
		gpio_mode_setup(DEFAULT_IIS_REC_GPIO_PORT_CLK_SD, GPIO_MODE_AF,
						GPIO_PUPD_NONE, DEFAULT_IIS_REC_GPIO_CLK | DEFAULT_IIS_REC_GPIO_SD);

		gpio_set_output_options(DEFAULT_IIS_REC_GPIO_PORT_WS, GPIO_OTYPE_PP,
								GPIO_OSPEED_50MHZ, DEFAULT_IIS_REC_GPIO_WS);
		gpio_set_output_options(DEFAULT_IIS_REC_GPIO_PORT_CLK_SD, GPIO_OTYPE_PP,
								GPIO_OSPEED_50MHZ, DEFAULT_IIS_REC_GPIO_CLK);
		//gpio_mode_setup(DEFAULT_IIS_REC_GPIO_PORT_CLK_SD, );

		gpio_set_af(DEFAULT_IIS_REC_GPIO_PORT_WS, GPIO_AF6,
					DEFAULT_IIS_REC_GPIO_WS);
		gpio_set_af(DEFAULT_IIS_REC_GPIO_PORT_CLK_SD, GPIO_AF6,
					DEFAULT_IIS_REC_GPIO_CLK | DEFAULT_IIS_REC_GPIO_SD);
	} else {
		/* Deinit gpio of IIS */
		//rcc_periph_clock_disable(DEFAULT_IIS_GPIO_RCC);
	}
}

static struct yiis_ctrl _yiis_2_ctrl = {
	.iis_rcc = RCC_SPI2,
	.iis_rcc_rst = RST_SPI2,
	.iis_base = SPI2,
	.iis_nvic_irq = NVIC_SPI2_IRQ,
	.iis_gpio_init = _yiis_2_gpio_init,

#if (DEFAULT_IIS_USE_DMA == 1)
	.iis_dma_rx_rcc = RCC_DMA1,
	.iis_dma_rx_base = DMA1,
	.iis_dma_rx_stream = DMA_STREAM3,
	.iis_dma_rx_chsel = DMA_SxCR_CHSEL_0,
	.iis_dma_rx_mem_size = DMA_SxCR_MSIZE_16BIT,
	.iis_dma_rx_peri_size = DMA_SxCR_PSIZE_16BIT,
	.iis_dma_rx_peri_addr = (uint32_t)(&SPI2_DR),
	.iis_dma_rx_nvic_irq = NVIC_DMA1_STREAM3_IRQ,

	.iis_dma_tx_rcc = RCC_DMA1,
	.iis_dma_tx_base = DMA1,
	.iis_dma_tx_stream = DMA_STREAM4,
	.iis_dma_tx_chsel = DMA_SxCR_CHSEL_0,
	.iis_dma_tx_mem_size = DMA_SxCR_MSIZE_16BIT,
	.iis_dma_tx_peri_size = DMA_SxCR_PSIZE_16BIT,
	.iis_dma_tx_peri_addr = (uint32_t)(&SPI2_DR),
	.iis_dma_tx_nvic_irq = NVIC_DMA1_STREAM4_IRQ,

	.sampling_rate = 0,
	.channels = 0,
	.bit_depth = 0,
	.transfer_bit_width = 8,
	.audio_standard = IIS_AUDIO_STANDARD_PHILIPS_STANDARD
#endif
};

static struct yiis_ctrl _yiis_3_ctrl = {
	.iis_rcc = RCC_SPI3,
	.iis_rcc_rst = RST_SPI3,
	.iis_base = SPI3,
	.iis_nvic_irq = NVIC_SPI3_IRQ,
	.iis_gpio_init = _yiis_3_gpio_init,

#if (DEFAULT_IIS_USE_DMA == 1)
	.iis_dma_rx_rcc = RCC_DMA1,
	.iis_dma_rx_base = DMA1,
	.iis_dma_rx_stream = DMA_STREAM2,
	.iis_dma_rx_chsel = DMA_SxCR_CHSEL_0,
	.iis_dma_rx_mem_size = DMA_SxCR_MSIZE_16BIT,
	.iis_dma_rx_peri_size = DMA_SxCR_PSIZE_16BIT,
	.iis_dma_rx_peri_addr = (uint32_t)(&SPI3_DR),
	.iis_dma_rx_nvic_irq = NVIC_DMA1_STREAM2_IRQ,

	.iis_dma_tx_rcc = RCC_DMA1,
	.iis_dma_tx_base = DMA1,
	.iis_dma_tx_stream = DMA_STREAM5,
	.iis_dma_tx_chsel = DMA_SxCR_CHSEL_0,
	.iis_dma_tx_mem_size = DMA_SxCR_MSIZE_16BIT,
	.iis_dma_tx_peri_size = DMA_SxCR_PSIZE_16BIT,
	.iis_dma_tx_peri_addr = (uint32_t)(&SPI3_DR),
	.iis_dma_tx_nvic_irq = NVIC_DMA1_STREAM5_IRQ,

	.sampling_rate = 0,
	.channels = 0,
	.bit_depth = 0,
	.transfer_bit_width = 8,
	.audio_standard = IIS_AUDIO_STANDARD_PHILIPS_STANDARD
#endif
};

struct yiis_ctrl *YIIS_2_CTRL = &_yiis_2_ctrl;
struct yiis_ctrl *YIIS_3_CTRL = &_yiis_3_ctrl;


int yiis_init(struct yiis_ctrl *iis)
{
	int ret = -1;
	if (iis == NULL) {
		goto para_err;
	}

	ret = 0;

para_err:
	return ret;
}

int yiis_deinit(struct yiis_ctrl *iis)
{
	int ret = -1;
	if (iis == NULL) {
		goto para_err;
	}

	iis->is_dma_working = 0;
	SPI_I2SCFGR(iis->iis_base) &= ~SPI_I2SCFGR_I2SE;

	//nvic_disable_irq(iis->iis_nvic_irq);

	if (iis->iis_gpio_init != NULL) {
		iis->iis_gpio_init(0);
	}

#if (DEFAULT_IIS_USE_DMA == 1)
	rcc_periph_clock_disable(iis->iis_dma_rx_rcc);
	rcc_periph_clock_disable(iis->iis_dma_tx_rcc);
	nvic_disable_irq(iis->iis_dma_rx_nvic_irq);
	nvic_disable_irq(iis->iis_dma_tx_nvic_irq);
	iis->iis_dma_rx_enabled = 0;
	iis->iis_dma_tx_enabled = 0;
	iis->sampling_rate = 0;
	iis->bit_depth = 0;
	iis->transfer_bit_width = 8;
	iis->channels = 0;
	iis->audio_standard = IIS_AUDIO_STANDARD_PHILIPS_STANDARD;
#endif

	rcc_osc_off(RCC_PLLI2S);
	rcc_periph_clock_disable(iis->iis_rcc);
	rcc_periph_reset_pulse(iis->iis_rcc_rst);

	ret = 0;

para_err:
	return ret;
}

int yiis_config(struct yiis_ctrl *iis, enum yiis_dma_direction dir,
				uint32_t sampling_rate, uint8_t channels, uint8_t bit_depth,
				enum IIS_AUDIO_STANDARD audio_standard)
{
	int ret = -1;
	enum IIS_CLOCK_TARGET_HZ iis_smr;
	enum IIS_DATA_FORMAT iis_dfmt;

	if (iis == NULL || (dir != YIIS_DMA_DIRECTION_RX && dir != YIIS_DMA_DIRECTION_TX)) {
		goto para_err;
	}

	iis->bit_depth = 0;
	iis->transfer_bit_width = 8;
	iis->channels = 0;
	iis->sampling_rate = 0;

	if (iis == NULL || sampling_rate == 0 || channels == 0 || bit_depth == 0
		|| audio_standard <= IIS_AUDIO_STANDARD_INVALID
		|| audio_standard >= IIS_AUDIO_STANDARD_MAX) {
		goto para_err;
	}

	if (bit_depth == 16) {
		iis_dfmt = IIS_DATA_FORMAT_16BIT;
		iis->iis_dma_rx_mem_size = DMA_SxCR_MSIZE_16BIT;
		iis->iis_dma_rx_peri_size = DMA_SxCR_PSIZE_16BIT;
		iis->iis_dma_tx_mem_size = DMA_SxCR_MSIZE_16BIT;
		iis->iis_dma_tx_peri_size = DMA_SxCR_PSIZE_16BIT;
		iis->transfer_bit_width = 16;
	} else if (bit_depth == 24) {
		iis_dfmt = IIS_DATA_FORMAT_32BIT;
		iis->iis_dma_rx_mem_size = DMA_SxCR_MSIZE_16BIT;
		iis->iis_dma_rx_peri_size = DMA_SxCR_PSIZE_16BIT;
		iis->iis_dma_tx_mem_size = DMA_SxCR_MSIZE_16BIT;
		iis->iis_dma_tx_peri_size = DMA_SxCR_PSIZE_16BIT;
		iis->transfer_bit_width = 16;
	} else if (bit_depth == 32) {
		iis_dfmt = IIS_DATA_FORMAT_32BIT;
		iis->iis_dma_rx_mem_size = DMA_SxCR_MSIZE_16BIT;
		iis->iis_dma_rx_peri_size = DMA_SxCR_PSIZE_16BIT;
		iis->iis_dma_tx_mem_size = DMA_SxCR_MSIZE_16BIT;
		iis->iis_dma_tx_peri_size = DMA_SxCR_PSIZE_16BIT;
		iis->transfer_bit_width = 16;
	} else {
		goto para_err;
	}
	iis->bit_depth = bit_depth;

	if (channels != 2) {
		goto para_err;
	}
	iis->channels = 2;

	switch (sampling_rate) {
	case 8000:
		iis_smr = IIS_CLOCK_TARGET_HZ_8000;
		break;
	case 16000:
		iis_smr = IIS_CLOCK_TARGET_HZ_16000;
		break;
	case 32000:
		iis_smr = IIS_CLOCK_TARGET_HZ_32000;
		break;
	case 48000:
		iis_smr = IIS_CLOCK_TARGET_HZ_48000;
		break;
	case 96000:
		iis_smr = IIS_CLOCK_TARGET_HZ_96000;
		break;
	case 22050:
		iis_smr = IIS_CLOCK_TARGET_HZ_22050;
		break;
	case 44100:
		iis_smr = IIS_CLOCK_TARGET_HZ_44100;
		break;
	case 192000:
		iis_smr = IIS_CLOCK_TARGET_HZ_192000;
		break;
	default:
		goto para_err;
	}
	iis->sampling_rate = sampling_rate;

	iis->audio_standard = audio_standard;

	struct IIS_CLOCK_SETTING *target_setting =
							*(STM32F401RC_IIS_CLOCK_SETTINGS + iis_smr) + iis_dfmt;
	rcc_osc_off(RCC_PLLI2S);
	while (rcc_is_osc_ready(RCC_PLLI2S)) {
		/* Wait for PLLI2S not ready */
	}
	rcc_plli2s_config(target_setting->PLLI2SN, target_setting->PLLI2SR);
	rcc_osc_on(RCC_PLLI2S);
	rcc_wait_for_osc_ready(RCC_PLLI2S);

	rcc_periph_clock_enable(iis->iis_rcc);

#if (DEFAULT_IIS_USE_DMA == 1)
	rcc_periph_clock_enable(iis->iis_dma_rx_rcc);
	rcc_periph_clock_enable(iis->iis_dma_tx_rcc);
	nvic_enable_irq(iis->iis_dma_rx_nvic_irq);
	nvic_enable_irq(iis->iis_dma_tx_nvic_irq);
	iis->iis_dma_rx_enabled = 0;
	iis->iis_dma_tx_enabled = 0;
	iis->is_dma_working = 0;
#endif

	if (iis->iis_gpio_init != NULL) {
		iis->iis_gpio_init(1);
	}

	//nvic_enable_irq(iis->iis_nvic_irq);
	SPI_I2SCFGR(iis->iis_base) &= ~SPI_I2SCFGR_I2SE;
	SPI_I2SCFGR(iis->iis_base) |= SPI_I2SCFGR_I2SMOD;
	if (dir == YIIS_DMA_DIRECTION_RX) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE << SPI_I2SCFGR_I2SCFG_LSB);
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB);
	}

	if (audio_standard == IIS_AUDIO_STANDARD_PHILIPS_STANDARD) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_I2SSTD_I2S_PHILIPS << SPI_I2SCFGR_I2SSTD_LSB);
	} else if (audio_standard == IIS_AUDIO_STANDARD_MSB_JUSTIFIED) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_I2SSTD_MSB_JUSTIFIED << SPI_I2SCFGR_I2SSTD_LSB);
	} else if (audio_standard == IIS_AUDIO_STANDARD_LSB_JUSTIFIED) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_I2SSTD_LSB_JUSTIFIED << SPI_I2SCFGR_I2SSTD_LSB);
	} else if (audio_standard == IIS_AUDIO_STANDARD_PCM_STANDARD) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_I2SSTD_PCM << SPI_I2SCFGR_I2SSTD_LSB);
	}

	if (bit_depth == 16) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_DATLEN_16BIT << SPI_I2SCFGR_DATLEN_LSB);
		SPI_I2SCFGR(iis->iis_base) &= ~SPI_I2SCFGR_CHLEN;
	} else if (bit_depth == 24) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_DATLEN_24BIT << SPI_I2SCFGR_DATLEN_LSB);
		SPI_I2SCFGR(iis->iis_base) |= SPI_I2SCFGR_CHLEN;
	} else if (bit_depth == 32) {
		SPI_I2SCFGR(iis->iis_base) |= (SPI_I2SCFGR_DATLEN_32BIT << SPI_I2SCFGR_DATLEN_LSB);
		SPI_I2SCFGR(iis->iis_base) |= SPI_I2SCFGR_CHLEN;
	}

	//SPI_I2SCFGR(iis->iis_base) |= SPI_I2SCFGR_CKPOL;
	SPI_I2SPR(iis->iis_base) |= (target_setting->I2SODD << 8 | target_setting->I2SDIV);
	SPI_I2SCFGR(iis->iis_base) |= SPI_I2SCFGR_I2SE;

	iis->is_dma_working = 1;

	ret = 0;

para_err:
	return ret;
}

#if (DEFAULT_IIS_USE_DMA == 1)
static void _yiis_start_rx_dma(struct yiis_ctrl *iis, void *buf, uint16_t transfer_size)
{
	if (iis == NULL || buf == NULL || transfer_size == 0 || iis->is_dma_working == 0) {
		goto para_err;
	}

	iis->iis_dma_rx_enabled = 1;

	dma_stream_reset(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_enable_double_buffer_mode(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_set_peripheral_address(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, iis->iis_dma_rx_peri_addr);
	dma_set_memory_address(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, (uint32_t)buf);
	dma_set_memory_address_1(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, (uint32_t)buf);
	dma_set_number_of_data(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, transfer_size / (iis->transfer_bit_width / 8));
	dma_channel_select(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, iis->iis_dma_rx_chsel);
	dma_set_dma_flow_control(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	//dma_set_priority(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, DMA_SxCR_PL_MEDIUM);
	dma_set_transfer_mode(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
	dma_enable_memory_increment_mode(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_disable_peripheral_increment_mode(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_set_memory_size(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, iis->iis_dma_rx_mem_size);
	dma_set_peripheral_size(iis->iis_dma_rx_base, iis->iis_dma_rx_stream, iis->iis_dma_rx_peri_size);
	dma_enable_transfer_complete_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_enable_half_transfer_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_enable_transfer_error_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	dma_enable_stream(iis->iis_dma_rx_base ,iis->iis_dma_rx_stream);
	spi_enable_rx_dma(iis->iis_base);

para_err:
	return;
}

static void _yiis_start_tx_dma(struct yiis_ctrl *iis, void *addr, uint16_t transfer_size)
{
	if (iis == NULL || addr == NULL || transfer_size == 0 || iis->is_dma_working == 0) {
		goto para_err;
	}

	iis->iis_dma_tx_enabled = 1;

	dma_stream_reset(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_enable_double_buffer_mode(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_set_peripheral_address(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, iis->iis_dma_tx_peri_addr);
	dma_set_memory_address(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, (uint32_t)addr);
	dma_set_memory_address_1(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, (uint32_t)addr);
	dma_set_number_of_data(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, transfer_size / (iis->transfer_bit_width / 8));
	dma_channel_select(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, iis->iis_dma_tx_chsel);
	dma_set_dma_flow_control(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	//dma_set_priority(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, DMA_SxCR_PL_MEDIUM);
	dma_set_transfer_mode(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_enable_memory_increment_mode(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_disable_peripheral_increment_mode(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_set_memory_size(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, iis->iis_dma_tx_mem_size);
	dma_set_peripheral_size(iis->iis_dma_tx_base, iis->iis_dma_tx_stream, iis->iis_dma_tx_peri_size);
	dma_enable_transfer_complete_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_enable_half_transfer_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_enable_transfer_error_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	dma_enable_stream(iis->iis_dma_tx_base ,iis->iis_dma_tx_stream);
	spi_enable_tx_dma(iis->iis_base);

para_err:
	return;
}

/* IIS_2_DMA_RX isr */
void dma1_stream3_isr(void)
{
	if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_TCIF)) {
		/* Transfer complete */
		dma_disable_transfer_complete_interrupt(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream);
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_TCIF);

	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_HTIF)) {
		/* Half transferred */
		dma_disable_half_transfer_interrupt(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream);
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_HTIF);

	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_TEIF)) {
		/* Transfer error */
		dma_disable_transfer_error_interrupt(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream);
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_TEIF);

#if 0
	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_DMEIF);
		dma_disable_stream(YIIS_2_CTRL->iis_dma_rx_base ,YIIS_2_CTRL->iis_dma_rx_stream);

	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_rx_base, YIIS_2_CTRL->iis_dma_rx_stream, DMA_FEIF);
		dma_disable_stream(YIIS_2_CTRL->iis_dma_rx_base ,YIIS_2_CTRL->iis_dma_rx_stream);

	} else {
#endif
	}
}

/* IIS_3_DMA_RX isr */
void dma1_stream2_isr(void)
{
	if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_TCIF)) {
		/* Transfer complete */
		//dma_disable_transfer_complete_interrupt(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream);
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_TCIF);

		YRingBufferReturnTheBlankItemAddress(YIIS_3_CTRL->dma_rx_ringbuf);
		void *next_addr = YRingBufferTakeAnBlankItemAddress(YIIS_3_CTRL->dma_rx_ringbuf, 1);

		if (dma_get_target(YIIS_3_CTRL->iis_dma_rx_base,
						YIIS_3_CTRL->iis_dma_rx_stream) == 0) {
			dma_set_memory_address_1(YIIS_3_CTRL->iis_dma_rx_base,
									YIIS_3_CTRL->iis_dma_rx_stream,
									(uint32_t)next_addr);
		} else {
			dma_set_memory_address(YIIS_3_CTRL->iis_dma_rx_base,
								YIIS_3_CTRL->iis_dma_rx_stream,
								(uint32_t)next_addr);
		}

	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_HTIF)) {
		/* Half transferred */
		dma_disable_half_transfer_interrupt(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream);
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_HTIF);

	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_TEIF)) {
		/* Transfer error */
		dma_disable_transfer_error_interrupt(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream);
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_TEIF);

#if 0
	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_DMEIF);
		dma_disable_stream(YIIS_3_CTRL->iis_dma_rx_base ,YIIS_3_CTRL->iis_dma_rx_stream);

	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_rx_base, YIIS_3_CTRL->iis_dma_rx_stream, DMA_FEIF);
		dma_disable_stream(YIIS_3_CTRL->iis_dma_rx_base ,YIIS_3_CTRL->iis_dma_rx_stream);

	} else {
#endif
	}
}

/* IIS_2_DMA_TX isr */
void dma1_stream4_isr(void)
{
	if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_TCIF)) {
		/* Transfer complete */
		//dma_disable_transfer_complete_interrupt(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream);
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_TCIF);

		uint8_t *next_addr = (uint8_t *)YRingBufferTakeAnItemAddressInCritical(YIIS_2_CTRL->dma_tx_ringbuf);
		if (next_addr != NULL) {
#if (IIS_DMA_WAIT_STATICSTIC == 1)
			YIIS_2_CTRL->real_data_come = 1;
#endif
			if (YRingBufferGetTakenItemNumberInCritical(YIIS_2_CTRL->dma_tx_ringbuf) > 2) {
				YRingBufferReturnTheItemAddressInCritical(YIIS_2_CTRL->dma_tx_ringbuf);
			}

			//YIIS_DBG("DMA got item addr: 0x%08X, index=%d\n",
			//		next_addr,
			//		((void *)next_addr - YIIS_2_CTRL->dma_tx_ringbuf->data) / YIIS_2_CTRL->dma_tx_ringbuf->size_of_item);
		} else {
#if (IIS_DMA_WAIT_STATICSTIC == 1)
			if (YIIS_2_CTRL->real_data_come) {
				YIIS_2_CTRL->dma_buffer_not_ready_l++;
				if (YIIS_2_CTRL->dma_buffer_not_ready_l == 0) {
					YIIS_2_CTRL->dma_buffer_not_ready_h++;
				}
			}
#endif
			//YIIS_DBG("DMA got NULL rb item addr\n");
		}

		if (dma_get_target(YIIS_2_CTRL->iis_dma_tx_base,
						YIIS_2_CTRL->iis_dma_tx_stream) == 0) {
			dma_set_memory_address_1(YIIS_2_CTRL->iis_dma_tx_base,
									YIIS_2_CTRL->iis_dma_tx_stream,
									(uint32_t)next_addr);
		} else {
			dma_set_memory_address(YIIS_2_CTRL->iis_dma_tx_base,
								YIIS_2_CTRL->iis_dma_tx_stream,
								(uint32_t)next_addr);
		}

		//dma_enable_transfer_complete_interrupt(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream);

	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_HTIF)) {
		/* Half transferred */
		dma_disable_half_transfer_interrupt(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream);
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_HTIF);

	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_TEIF)) {
		/* Transfer error */
		dma_disable_transfer_error_interrupt(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream);
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_TEIF);

#if 0
	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_DMEIF);
		dma_disable_stream(YIIS_2_CTRL->iis_dma_tx_base ,YIIS_2_CTRL->iis_dma_tx_stream);

	} else if (dma_get_interrupt_flag(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(YIIS_2_CTRL->iis_dma_tx_base, YIIS_2_CTRL->iis_dma_tx_stream, DMA_FEIF);
		dma_disable_stream(YIIS_2_CTRL->iis_dma_tx_base ,YIIS_2_CTRL->iis_dma_tx_stream);

	} else {
#endif
	}
}


/* IIS_3_DMA_TX isr */
void dma1_stream5_isr(void)
{
	if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_TCIF)) {
		/* Transfer complete */
		dma_disable_transfer_complete_interrupt(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream);
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_TCIF);

	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_HTIF)) {
		/* Half transferred */
		dma_disable_half_transfer_interrupt(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream);
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_HTIF);

	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_TEIF)) {
		/* Transfer error */
		dma_disable_transfer_error_interrupt(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream);
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_TEIF);

#if 0
	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_DMEIF)) {
		/* Direct mode error */
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_DMEIF);
		dma_disable_stream(YIIS_3_CTRL->iis_dma_tx_base ,YIIS_3_CTRL->iis_dma_tx_stream);

	} else if (dma_get_interrupt_flag(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_FEIF)) {
		/* FIFO error */
		dma_clear_interrupt_flags(YIIS_3_CTRL->iis_dma_tx_base, YIIS_3_CTRL->iis_dma_tx_stream, DMA_FEIF);
		dma_disable_stream(YIIS_3_CTRL->iis_dma_tx_base ,YIIS_3_CTRL->iis_dma_tx_stream);

	} else {
#endif
	}
}
#endif


/* IIS_2 isr */
#if 0
void spi2_isr()
{
	if ((SPI_SR(YIIS_2_CTRL->iis_base) & SPI_SR_TXE) &&
		!(SPI_SR(YIIS_2_CTRL->iis_base) & SPI_SR_BSY)) {
		spi_disable_tx_buffer_empty_interrupt(YIIS_2_CTRL->iis_base);

		if (YIIS_2_CTRL->on_dma_tx_completed != NULL) {
			YIIS_2_CTRL->on_dma_tx_completed();
		}
	}
}


static void _yiis_on_dma_rx_complete(void)
{
}

static void _yiis_on_dma_tx_complete(void)
{
#if 0
	if (YIIS_2_CTRL->addr_sending != YIIS_2_CTRL->dummy_buffer) {
		YRingBufferReturnTheItemAddressInCritical(YIIS_2_CTRL->dma_tx_ringbuf, YIIS_2_CTRL->addr_sending);
	}

	if (YIIS_2_CTRL->is_dma_working) {
		YIIS_2_CTRL->addr_sending = YRingBufferTakeAnItemAddressInCritical(YIIS_2_CTRL->dma_tx_ringbuf);
		if (YIIS_2_CTRL->addr_sending == NULL) {
			YIIS_2_CTRL->addr_sending = YIIS_2_CTRL->dummy_buffer;
			YIIS_2_CTRL->size_sending = YIIS_2_CTRL->dummy_buffer_size;
		} else {
			YIIS_2_CTRL->size_sending = YRingBufferGetItemSizeInCritical(YIIS_2_CTRL->dma_tx_ringbuf);
		}

		_yiis_start_tx_dma(YIIS_2_CTRL, YIIS_2_CTRL->addr_sending, YIIS_2_CTRL->size_sending / 2);
	} else {
		/* DMA stopped, do nothing */
	}
#endif
}
#endif

int yiis_dma_start(struct yiis_ctrl *iis, enum yiis_dma_direction dir,
				struct YRingBuffer *rb, uint16_t data_length)
{
	int ret = -1;
	if (iis == NULL || (dir != YIIS_DMA_DIRECTION_RX && dir != YIIS_DMA_DIRECTION_TX)
		|| rb == NULL || data_length == 0 || iis->is_dma_working == 0) {
		goto para_err;
	}

	YRingBufferClear(rb);

	if (dir == YIIS_DMA_DIRECTION_RX) {
		iis->dma_rx_ringbuf = rb;
		_yiis_start_rx_dma(iis, YRingBufferTakeAnBlankItemAddress(iis->dma_rx_ringbuf, 1), data_length);
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		iis->dma_tx_ringbuf = rb;
		_yiis_start_tx_dma(iis, rb->data, data_length);
	}

#if (IIS_DMA_WAIT_STATICSTIC == 1)
		iis->real_data_come = 0;
		iis->dma_buffer_not_ready_h = 0;
		iis->dma_buffer_not_ready_l = 0;
#endif

	ret = 0;

para_err:
	return ret;
}

int yiis_dma_end(struct yiis_ctrl *iis, enum yiis_dma_direction dir)
{
	int ret = -1;
	if (iis == NULL || (dir != YIIS_DMA_DIRECTION_RX && dir != YIIS_DMA_DIRECTION_TX)
		|| iis->is_dma_working == 0) {
		goto para_err;
	}

	if (dir == YIIS_DMA_DIRECTION_RX) {
		spi_disable_rx_dma(iis->iis_base);
		dma_disable_stream(iis->iis_dma_rx_base ,iis->iis_dma_rx_stream);
		dma_disable_transfer_complete_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		dma_disable_half_transfer_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		dma_disable_transfer_error_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		iis->iis_dma_rx_enabled = 0;
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		spi_disable_tx_dma(iis->iis_base);
		dma_disable_stream(iis->iis_dma_tx_base ,iis->iis_dma_tx_stream);
		dma_disable_transfer_complete_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		dma_disable_half_transfer_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		dma_disable_transfer_error_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		iis->iis_dma_tx_enabled = 0;
	}

	ret = 0;

para_err:
	return ret;
}

void yiis_dma_disable_interrupts(struct yiis_ctrl *iis, enum yiis_dma_direction dir)
{
	if (iis == NULL || iis->is_dma_working == 0) {
		return;
	}

	if (dir == YIIS_DMA_DIRECTION_RX) {
		dma_disable_transfer_complete_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		dma_disable_half_transfer_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		dma_disable_transfer_error_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		dma_disable_transfer_complete_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		dma_disable_half_transfer_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		dma_disable_transfer_error_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	}
}

void yiis_dma_enable_interrupts(struct yiis_ctrl *iis, enum yiis_dma_direction dir)
{
	if (iis == NULL || iis->is_dma_working == 0) {
		return;
	}

	if (dir == YIIS_DMA_DIRECTION_RX) {
		dma_enable_transfer_complete_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		dma_enable_half_transfer_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		dma_enable_transfer_error_interrupt(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		dma_enable_transfer_complete_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		dma_enable_half_transfer_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		dma_enable_transfer_error_interrupt(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
	}
}

void yiis_dma_pause(struct yiis_ctrl *iis, enum yiis_dma_direction dir)
{
	if (iis == NULL || iis->is_dma_working == 0) {
		return;
	}
	if (dir == YIIS_DMA_DIRECTION_RX) {
		if (iis->iis_dma_rx_enabled) {
			iis->iis_dma_rx_enabled = 0;
			dma_disable_stream(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		}
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		if (iis->iis_dma_tx_enabled) {
			iis->iis_dma_tx_enabled = 0;
			dma_disable_stream(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		}
	}
}

void yiis_dma_restore(struct yiis_ctrl *iis, enum yiis_dma_direction dir)
{
	if (iis == NULL || iis->is_dma_working == 0) {
		return;
	}
	if (dir == YIIS_DMA_DIRECTION_RX) {
		if (!iis->iis_dma_rx_enabled) {
			iis->iis_dma_rx_enabled = 1;
			dma_enable_stream(iis->iis_dma_rx_base, iis->iis_dma_rx_stream);
		}
	} else if (dir == YIIS_DMA_DIRECTION_TX) {
		if (!iis->iis_dma_tx_enabled) {
			iis->iis_dma_tx_enabled = 1;
			dma_enable_stream(iis->iis_dma_tx_base, iis->iis_dma_tx_stream);
		}
	}
}

int32_t yiis_transfer_data(struct yiis_ctrl *iis, uint8_t *data, uint16_t data_length)
{
	int32_t ret = -1;
	uint16_t buff_length;
	uint16_t real_length_transfered;
	if (iis == NULL || data == NULL) {
		goto para_err;
	}
	if (iis->is_dma_working == 0) {
		goto dma_not_start;
	}

	void *addr = YRingBufferTakeAnBlankItemAddress(iis->dma_tx_ringbuf, 0);
	if (addr == NULL) {
		goto no_buffer_available;
	}

	//YIIS_DBG("Got rb blank item addr: 0x%08X, index=%d, data_len=%d\n",
	//		addr, (addr - iis->dma_tx_ringbuf->data) / iis->dma_tx_ringbuf->size_of_item, data_length);
	buff_length = YRingBufferGetItemSize(iis->dma_tx_ringbuf);
	if (data_length > buff_length) {
		real_length_transfered = buff_length;
	} else {
		real_length_transfered = data_length;
	}
	memcpy(addr, data, real_length_transfered);

	if (YRingBufferReturnTheBlankItemAddress(iis->dma_tx_ringbuf) != 0) {
		goto blank_item_return_failed;
	}

	ret = real_length_transfered;

blank_item_return_failed:
no_buffer_available:
dma_not_start:
para_err:
	return ret;
}

int32_t yiis_receive_data(struct yiis_ctrl *iis, uint8_t *buf, uint16_t length)
{
	int32_t ret = -1;
	uint16_t buff_length;
	uint16_t real_length_received;
	if (iis == NULL || buf == NULL) {
		goto para_err;
	}
	if (iis->is_dma_working == 0) {
		goto dma_not_start;
	}

	void *addr = YRingBufferTakeAnItemAddress(iis->dma_rx_ringbuf);
	if (addr == NULL) {
		goto no_data_available;
	}
	buff_length = YRingBufferGetItemSize(iis->dma_rx_ringbuf);
	if (length > buff_length) {
		real_length_received = buff_length;
	} else {
		real_length_received = length;
	}
	memcpy(buf, addr, real_length_received);
	if (YRingBufferReturnTheItemAddress(iis->dma_rx_ringbuf) != 0) {
		goto return_item_failed;
	}

	ret = real_length_received;

return_item_failed:
no_data_available:
dma_not_start:
para_err:
	return ret;
}
