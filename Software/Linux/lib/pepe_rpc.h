#ifndef __PEPE_RPC_H_
#define __PEPE_RPC_H_


typedef struct
{
	unsigned int v_water;
	float t_water;
	unsigned int v_fan;
	unsigned int p_fan;
} rpc_data_t;

int read_data(char*,rpc_data_t*);
int write_fanspeed(char*,unsigned char);
void err_msg(char*);

#endif
