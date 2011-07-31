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

/** \file
 *
 *  Header file for Keyboard.c.
 */

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>
		#include <stdbool.h>
		#include <string.h>

		#include "Descriptors.h"

		#include <LUFA/Version.h>
//		#include <LUFA/Drivers/Board/Joystick.h>
//		#include <LUFA/Drivers/Board/LEDs.h>
//		#include <LUFA/Drivers/Board/Buttons.h>
		#include <LUFA/Drivers/USB/USB.h>

	/* Macros: */
		#define BTN_1 (1<<1)
		#define BTN_2 (1<<2)
		#define BTN_3 (1<<3)
		#define BTN_4 (1<<4)
		#define BTN_5 (1<<5)
		#define BTN_6 (1<<6)
		#define BTN_7 (1<<7)
		#define WHL_1_CW (1<<0)
		#define WHL_1_CCW (1<<1)
		#define WHL_2_CW (1<<2)
		#define WHL_2_CCW (1<<3)
		#define WHL1A (1<<4)
		#define WHL1B (1<<5)
		#define WHL2A (1<<6)
		#define WHL2B (1<<7)
		#define INT_TEMP_FLAG  2
		#define INT_FLOW_FLAG 1
		#define INT_FAN_FLAG 0

		typedef struct
		{
			uint16_t v_water;
			int16_t  t_water;
			uint16_t v_fan;
			uint8_t p_fan;
		} ATTR_PACKED USB_RPCReport_Data_t;

	/* Function Prototypes: */
		void SetupHardware(void);

		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);
		//void EVENT_USB_Device_StartOfFrame(void);

		bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
		                                         uint8_t* const ReportID,
		                                         const uint8_t ReportType,
		                                         void* ReportData,
		                                         uint16_t* const ReportSize);
		void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
		                                          const uint8_t ReportID,
		                                          const uint8_t ReportType,
		                                          const void* ReportData,
		                                          const uint16_t ReportSize);
		bool CALLBACK_RPC_Device_CreateRPCReport(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo,
										 uint8_t* const ReportID,
										 /*const uint8_t ReportType,*/
										 void* ReportData,
										 uint16_t* const ReportSize);
		void CALLBACK_RPC_Device_ProcessRPCReport(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo,
										  const uint8_t ReportID,
										  /*const uint8_t ReportType,*/
										  const void* ReportData,
										  const uint16_t ReportSize);
		void CheckButtonState(void);
		void Frontpanel_Init(void);
		void WriteButtonState(uint8_t*);
		void WriteWheelState(uint8_t*);
		void Waterflow_Init(void);
		void Waterflow_Worker(void);
		void Watertemp_Init(void);
void Watertemp_Worker(void);
		void Fans_Init(void);
		void Fans_Worker(void);
#endif

