#ifndef __I2C_H_
#define __I2C_H_

#include "yos_aht20.h"

#if (HOST_TYPE == 0)
#define I2C_F 100000UL
#elif (HOST_TYPE == 1)
#define I2C_F 400000UL
#endif

#define I2C_READ	1
#define I2C_WRITE	0

#define I2C_ACK		1
#define I2C_NACK	0

void iic_stub_init(void);
void iic_stub_start(int read_or_write);
void iic_stub_stop(void);
void iic_stub_write(uint8_t byte);
uint16_t iic_stub_read(void *buf, uint16_t buf_len);

#endif
