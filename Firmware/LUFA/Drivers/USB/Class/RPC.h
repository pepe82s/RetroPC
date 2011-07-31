#ifndef _RPC_CLASS_H_
#define _RPC_CLASS_H_

	/* Macros: */
		#define __INCLUDE_FROM_USB_DRIVER
		#define __INCLUDE_FROM_RPC_DRIVER

	/* Includes: */
		#include "../Core/USBMode.h"

		#if defined(USB_CAN_BE_DEVICE)
			#include "Device/RPC.h"
		#endif
/*
		#if defined(USB_CAN_BE_HOST)
			#include "Host/HID.h"
		#endif
*/
#endif

/** @} */

