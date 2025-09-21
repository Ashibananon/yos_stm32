#include <stdint.h>
#include "ssd1306xled.h"
#include "../../../src/ydevice/yiic.h"

void ssd1306_start_command(void)
{
	yiic_master_trans_start(YIIC_MASTER_TRANSMIT);
	yiic_master_select_slave(SSD1306_SADDR >> 1);
	uint8_t data = 0x00;
	yiic_master_transmit_data(&data, sizeof(data));
}

void ssd1306_start_data(void)
{
	yiic_master_trans_start(YIIC_MASTER_TRANSMIT);
	yiic_master_select_slave(SSD1306_SADDR >> 1);
	uint8_t data = 0x40;
	yiic_master_transmit_data(&data, sizeof(data));
}

void ssd1306_byte(uint8_t b)
{
	yiic_master_transmit_data(&b, sizeof(b));
}

void ssd1306_stop(void)
{
	yiic_master_trans_stop();
}
