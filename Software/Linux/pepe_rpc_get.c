#include "pepe_rpc_get.h"
#include <stdio.h>
#include <pepe_rpc.h>

int main(int argc, char **argv)
{
	char *device = "/dev/rpc";
	rpc_data_t data;
	read_data(device,&data);
	fprintf(stdout,"%d %.1f %d %d \n",data.v_water, data.t_water, data.v_fan, data.p_fan);
	return 0;

}
