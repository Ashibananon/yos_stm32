#include "aht20.h"
#include "i2c.h"
#include "../../../src/yos/yos.h"
#include "../../../src/ydevice/yiic.h"

void iic_stub_init(void)
{
}

void iic_stub_start(int read_or_write)
{
	yiic_master_trans_start(read_or_write == I2C_WRITE ? YIIC_MASTER_TRANSMIT : YIIC_MASTER_RECEIVE);
	yiic_master_select_slave(AHT20_ADDRESS);
}

void iic_stub_stop(void)
{
	yiic_master_trans_stop();
}

void iic_stub_write(uint8_t byte)
{
	yiic_master_transmit_data(&byte, sizeof(byte));
}

uint16_t iic_stub_read(void *buf, uint16_t buf_len)
{
	return yiic_master_receive_data(buf, buf_len);
}

#if 0
void _delay_ms(double __ms)
{
	uint16_t ms = (uint16_t)__ms;
	yos_task_msleep(ms);
}
#endif
