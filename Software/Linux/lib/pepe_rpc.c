#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "pepe_rpc.h"
#include "../../../rpc_global.h"

int read_data(char *filename, rpc_data_t *data)
{
	int fh;
	USB_RPCReport_Data_t buffer;
	
	/* open device */
	fh = open(filename,O_RDONLY);
	if( fh==-1 ) {
		err_msg("Error open devicen");
		return 0;
	}
	
	/* read values */
	if( read(fh, &buffer, sizeof(rpc_data_t))!=7 ) {
		err_msg("Error reading device");
		close(fh);
		return 0;
	}
	
	/* close device */
	close(fh);
	
	/* write values */
	data->v_water = buffer.v_water;
	data->t_water = buffer.t_water/10.0;
	data->v_fan = buffer.v_fan;
	data->p_fan = buffer.p_fan;
	
	return 1;
}

int write_fanspeed(char *filename,unsigned char fanspeed)
{
	int fh;

	if( fanspeed > 100 ) {
		err_msg("Fanspeed not valid");
		return 0;
	}

	/* open device */
	fh = open(filename,O_RDWR);
	if( fh==-1 ) {
		err_msg("Error open device");
		return 0;
	}

	/* write values */
	if( write(fh,&fanspeed,1) == -1 ) {
		err_msg("Error writing device");
		close(fh);
		return 0;
	}

	/* close device */
	close(fh);

	return 1;
}

void err_msg(char *msg)
{
	fprintf(stderr,"libpepe_rpc: %s\n",msg);
}
