/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YIIC_H_
#define _YIIC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum yiic_master_tr {
	YIIC_MASTER_TRANSMIT = 0,
	YIIC_MASTER_RECEIVE = 1
};

int yiic_master_init(uint32_t iic_freq);
int yiic_master_deinit(void);

void yiic_master_wait(void);
int yiic_master_trans_start(enum yiic_master_tr tran_type);
int yiic_master_trans_stop(void);
int yiic_master_select_slave(uint8_t addr);
uint16_t yiic_master_transmit_data(void *data, uint16_t data_len);
uint16_t yiic_master_receive_data(void *buff, uint16_t buf_len);

#ifdef __cplusplus
}
#endif
#endif
