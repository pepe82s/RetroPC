/*
             LUFA Library
     Copyright (C) Dean Camera, 2011.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#define  __INCLUDE_FROM_USB_DRIVER
#include "../../Core/USBMode.h"

#if defined(USB_CAN_BE_DEVICE)

#define  __INCLUDE_FROM_RPC_DRIVER
#define  __INCLUDE_FROM_RPC_DEVICE_C
#include "RPC.h"

void RPC_Device_ProcessControlRequest(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo)
{
	if (!(Endpoint_IsSETUPReceived()))
	  return;

	if (USB_ControlRequest.wIndex != RPCInterfaceInfo->Config.InterfaceNumber)
	  return;

	switch (USB_ControlRequest.bRequest)
	{
		/*case RPC_REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				uint16_t ReportSize = 0;
				uint8_t  ReportID   = (USB_ControlRequest.wValue & 0xFF);
				//uint8_t  ReportType = (USB_ControlRequest.wValue >> 8) - 1;
				uint8_t  ReportData[RPCInterfaceInfo->Config.ReportINEndpointSize];

				memset(ReportData, 0, sizeof(ReportData));

				CALLBACK_RPC_Device_CreateRPCReport(RPCInterfaceInfo, &ReportID, /*ReportType,*//* ReportData, &ReportSize);

				if (RPCInterfaceInfo->Config.PrevReportINBuffer != NULL)
				{
					memcpy(HIDInterfaceInfo->Config.PrevReportINBuffer, ReportData,
					       HIDInterfaceInfo->Config.PrevReportINBufferSize);
				}*//*
				
				Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);

				Endpoint_ClearSETUP();
				Endpoint_Write_Control_Stream_LE(ReportData, ReportSize);
				Endpoint_ClearOUT();
			}

			break;*/
		case RPC_REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				uint8_t ReportSize = USB_ControlRequest.wLength;
				uint8_t  ReportID   = (USB_ControlRequest.wValue & 0xFF);
				//uint8_t  ReportType = (USB_ControlRequest.wValue >> 8) - 1;
				uint8_t  ReportData[ReportSize];

				Endpoint_ClearSETUP();
				Endpoint_Read_Control_Stream_LE(ReportData, ReportSize);
				Endpoint_ClearIN();
				
				CALLBACK_RPC_Device_ProcessRPCReport(RPCInterfaceInfo, ReportID, /*ReportType,*/
				                                     &ReportData[ReportID ? 1 : 0], ReportSize - (ReportID ? 1 : 0));
			}

			break;
		/*case HID_REQ_GetProtocol:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_Write_8(HIDInterfaceInfo->State.UsingReportProtocol);
				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;
		case HID_REQ_SetProtocol:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				HIDInterfaceInfo->State.UsingReportProtocol = ((USB_ControlRequest.wValue & 0xFF) != 0x00);
			}

			break;
		case RPC_REQ_SetIdle:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_ClearStatusStage();

				RPCInterfaceInfo->State.IdleCount = ((USB_ControlRequest.wValue & 0xFF00) >> 6);
			}

			break;
		case RPC_REQ_GetIdle:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Endpoint_ClearSETUP();
				Endpoint_Write_8(RPCInterfaceInfo->State.IdleCount >> 2);
				Endpoint_ClearIN();
				Endpoint_ClearStatusStage();
			}

			break;*/
	}
}

bool RPC_Device_ConfigureEndpoints(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo)
{
	//memset(&RPCInterfaceInfo->State, 0x00, sizeof(RPCInterfaceInfo->State));
	//RPCInterfaceInfo->State.UsingReportProtocol = true;
	//RPCInterfaceInfo->State.IdleCount = 500;

	if (!(Endpoint_ConfigureEndpoint(RPCInterfaceInfo->Config.ReportINEndpointNumber, EP_TYPE_INTERRUPT,
									 ENDPOINT_DIR_IN, RPCInterfaceInfo->Config.ReportINEndpointSize,
									 RPCInterfaceInfo->Config.ReportINEndpointDoubleBank ? ENDPOINT_BANK_DOUBLE : ENDPOINT_BANK_SINGLE))
		)
	{
		return false;
	}

	return true;
}

void RPC_Device_USBTask(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo)
{
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	Endpoint_SelectEndpoint(RPCInterfaceInfo->Config.ReportINEndpointNumber);

	if (Endpoint_IsReadWriteAllowed())
	{
		uint8_t  ReportINData[RPCInterfaceInfo->Config.ReportINEndpointSize];
		uint8_t  ReportID     = 0;
		uint16_t ReportINSize = 0;

		//memset(ReportINData, 0, sizeof(ReportINData));

		CALLBACK_RPC_Device_CreateRPCReport(RPCInterfaceInfo, &ReportID, /*HID_REPORT_ITEM_In,*/
		                                                             ReportINData, &ReportINSize);
		//bool StatesChanged     = false;
		//bool IdlePeriodElapsed = (RPCInterfaceInfo->State.IdleCount && !(RPCInterfaceInfo->State.IdleMSRemaining));

		/*if (RPCInterfaceInfo->Config.PrevReportINBuffer != NULL)
		{
			StatesChanged = (memcmp(ReportINData, HIDInterfaceInfo->Config.PrevReportINBuffer, ReportINSize) != 0);
			memcpy(HIDInterfaceInfo->Config.PrevReportINBuffer, ReportINData, HIDInterfaceInfo->Config.PrevReportINBufferSize);
		}*/

		if (ReportINSize)
		{
			//RPCInterfaceInfo->State.IdleMSRemaining = RPCInterfaceInfo->State.IdleCount;

			Endpoint_SelectEndpoint(RPCInterfaceInfo->Config.ReportINEndpointNumber);

			if (ReportID)
			  Endpoint_Write_8(ReportID);

			Endpoint_Write_Stream_LE(ReportINData, ReportINSize, NULL);

			Endpoint_ClearIN();
		}
	}
}

#endif

