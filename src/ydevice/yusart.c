/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <libopencm3/cm3/cortex.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include "yusart.h"
#include "../../lib/ringbuf/yringbuffer.h"

#define DEFAULT_USART_PORT					USART1
#define DEFAULT_USART_RCC					RCC_USART1
#define DEFAULT_USART_GPIO_RCC				RCC_GPIOA
#define DEFAULT_USART_GPIO_BANK_TX			GPIO_BANK_USART1_TX
#define DEFAULT_USART_GPIO_BANK_RX			GPIO_BANK_USART1_RX
#define DEFAULT_USART_GPIO_TX				GPIO_USART1_TX
#define DEFAULT_USART_GPIO_RX				GPIO_USART1_RX
#define DEFAULT_USART_NVIC_IRQ				NVIC_USART1_IRQ

static char _yusart_receive_buffer[YUSART_RECEIVE_BUFFER_SIZE_BYTE];
static struct YRingBuffer _yusart_rx_rb;

static void yusart_interrupt_disable(void)
{
	usart_disable_rx_interrupt(DEFAULT_USART_PORT);
	//usart_disable_tx_interrupt(DEFAULT_USART_PORT);
	//usart_disable_tx_complete_interrupt(DEFAULT_USART_PORT);
	//usart_disable_idle_interrupt(DEFAULT_USART_PORT);
	//usart_disable_error_interrupt(DEFAULT_USART_PORT);
}

static void yusart_interrupt_enable(void)
{
	usart_enable_rx_interrupt(DEFAULT_USART_PORT);
	//usart_enable_tx_interrupt(DEFAULT_USART_PORT);
	//usart_enable_tx_complete_interrupt(DEFAULT_USART_PORT);
	//usart_enable_idle_interrupt(DEFAULT_USART_PORT);
	//usart_enable_error_interrupt(DEFAULT_USART_PORT);
}

static void yusart_init(uint32_t baudrate, uint8_t stop_bits)
{
	cm_disable_interrupts();

	rcc_periph_clock_enable(DEFAULT_USART_GPIO_RCC);
	rcc_periph_clock_enable(DEFAULT_USART_RCC);

	YRingBufferInit(&_yusart_rx_rb, _yusart_receive_buffer, sizeof(_yusart_receive_buffer));

	gpio_set_mode(DEFAULT_USART_GPIO_BANK_TX, GPIO_MODE_OUTPUT_50_MHZ,
				GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, DEFAULT_USART_GPIO_TX);
	gpio_set_mode(DEFAULT_USART_GPIO_BANK_RX, GPIO_MODE_INPUT,
				GPIO_CNF_INPUT_FLOAT, DEFAULT_USART_GPIO_RX);

	/* Setup UART parameters. */
	usart_set_baudrate(DEFAULT_USART_PORT, baudrate);
	usart_set_databits(DEFAULT_USART_PORT, USART_DATABIT);
	usart_set_stopbits(DEFAULT_USART_PORT, USART_STOPBITS_1);
	usart_set_mode(DEFAULT_USART_PORT, USART_MODE_TX_RX);
	usart_set_parity(DEFAULT_USART_PORT, USART_PARITY_NONE);
	usart_set_flow_control(DEFAULT_USART_PORT, USART_FLOWCONTROL_NONE);

	yusart_interrupt_enable();
	nvic_enable_irq(DEFAULT_USART_NVIC_IRQ);

	/* Finally enable the USART. */
	usart_enable(DEFAULT_USART_PORT);

	cm_enable_interrupts();
}

void usart1_isr(void)
{
	char d;
	if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_RXNE)) {
		/* Read data register not empty */
		d = usart_recv(DEFAULT_USART_PORT);
		YRingBufferPutData(&_yusart_rx_rb, &d, sizeof(d), 1);
#if 0
	} else if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_TXE)) {
		/* Transmit data buffer empty */
	} else if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_TC)) {
		/* Transmission complete */
	} else if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_ORE)) {
		/* Overrun error */
		usart_recv(DEFAULT_USART_PORT);
	} else if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_NE)) {
		/* Noise error */
		usart_recv(DEFAULT_USART_PORT);
	} else if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_FE)) {
		/* Framing error */
		usart_recv(DEFAULT_USART_PORT);
	} else if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_PE)) {
		/* Parity error */
		usart_recv(DEFAULT_USART_PORT);
#endif
	}
}

static void yusart_deinit(void)
{
	cm_disable_interrupts();

	usart_disable(DEFAULT_USART_PORT);
	nvic_disable_irq(NVIC_USART1_IRQ);
	yusart_interrupt_disable();
	YRingBufferDestory(&_yusart_rx_rb);

	rcc_periph_clock_disable(DEFAULT_USART_GPIO_RCC);
	rcc_periph_clock_disable(DEFAULT_USART_RCC);

	cm_enable_interrupts();
}

static int yusart_can_transmit(void)
{
	int ret;

	cm_disable_interrupts();
	//yusart_interrupt_disable();
	ret = usart_get_flag(DEFAULT_USART_PORT, USART_SR_TXE);
	//yusart_interrupt_enable();
	cm_enable_interrupts();

	return ret;
}

static int yusart_can_receive(void)
{
	int can;

	cm_disable_interrupts();
	//yusart_interrupt_disable();
	can = YRingBufferGetSize(&_yusart_rx_rb) > 0;
	//yusart_interrupt_enable();
	cm_enable_interrupts();

	return can;
}

static int yusart_io_init(void)
{
	yusart_init(USART_BAUDRATE, USART_STOPBITS);

	return 0;
}

static int yusart_io_deinit(void)
{
	yusart_deinit();

	return 0;
}

static int yusart_io_read_byte_no_block(uint8_t *b)
{
	int ret = -1;
	uint8_t data;

	cm_disable_interrupts();
	//yusart_interrupt_disable();
	if (YRingBufferGetData(&_yusart_rx_rb, &data, sizeof(data)) == sizeof(data)) {
		if (b != NULL) {
			*b = data;
		}
		ret = 0;
	}
	//yusart_interrupt_enable();
	cm_enable_interrupts();

	return ret;
}

static int yusart_io_write_byte_no_block(uint8_t b)
{
	int ret = -1;

	cm_disable_interrupts();
	//yusart_interrupt_disable();
	if (usart_get_flag(DEFAULT_USART_PORT, USART_SR_TXE)) {
		usart_send(DEFAULT_USART_PORT, b);
		ret = 0;
	}
	//yusart_interrupt_enable();
	cm_enable_interrupts();

	return ret;
}


static struct basic_io_port_operations _yusart_io_operations = {
	.basic_io_port_init = yusart_io_init,
	.basic_io_port_deinit = yusart_io_deinit,
	.basic_io_port_can_read = yusart_can_receive,
	.basic_io_port_read_byte_no_block = yusart_io_read_byte_no_block,
	.basic_io_port_can_write = yusart_can_transmit,
	.basic_io_port_write_byte_no_block = yusart_io_write_byte_no_block
};

struct basic_io_port_operations *yusart_io_operations = &_yusart_io_operations;
