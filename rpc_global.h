#ifndef _RPC_GLOBAL_H_
#define _RPC_GLOBAL_H_
#include <stdint.h>

typedef struct
{
	uint16_t v_water;
	int16_t t_water;
	uint16_t v_fan;
	uint8_t p_fan;
} __attribute__((__packed__)) USB_RPCReport_Data_t;


#endif
