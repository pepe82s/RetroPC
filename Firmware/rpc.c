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
 *  Main source file for the Keyboard demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "rpc.h"

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

static uint8_t btn_status = 0;
static uint8_t whl_status = 0;
static uint8_t fan_flow_status = 0;
static uint8_t fan_value;
static uint16_t timer_flow = 0, timer_fan = 0;
static int16_t adc_voltage = 0;
static USB_RPCReport_Data_t rpc_values =
{
	.v_water = 0,
	.t_water = 0,
	.v_fan   = 0,
	.p_fan   = 50,
};
/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
 	{
		.Config =
			{
				.InterfaceNumber              = 0,

				.ReportINEndpointNumber       = KEYBOARD_EPNUM,
				.ReportINEndpointSize         = KEYBOARD_EPSIZE,
				.ReportINEndpointDoubleBank   = false,

				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
    };

USB_ClassInfo_RPC_Device_t Values_RPC_Interface =
{
	.Config =
	{
		.InterfaceNumber              = 1,
		
		.ReportINEndpointNumber       = RPC_IN_EPNUM,
		.ReportINEndpointSize         = RPC_IN_EPSIZE,
		.ReportINEndpointDoubleBank   = false,
		
		/*.ReportOUTEndpointNumber       = RPC_OUT_EPNUM,
		.ReportOUTEndpointSize         = RPC_OUT_EPSIZE,
		.ReportOUTEndpointDoubleBank   = false,*/
	},
};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
		RPC_Device_USBTask(&Values_RPC_Interface);
		USB_USBTask();
		CheckButtonState();
		Waterflow_Worker();
		Watertemp_Worker();
		Fans_Worker();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable JTAG if enabled */
	MCUCR |= (1<<JTD);
	MCUCR |= (1<<JTD);

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	//Joystick_Init();
	//LEDs_Init();
	//Buttons_Init();
	Frontpanel_Init();
	Waterflow_Init();
	Watertemp_Init();
	Fans_Init();
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
	ConfigSuccess &= RPC_Device_ConfigureEndpoints(&Values_RPC_Interface);

	USB_Device_EnableSOFEvents();

	//LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
	RPC_Device_ProcessControlRequest(&Values_RPC_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
/*void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
	RPC_Device_MillisecondElapsed(&Values_RPC_Interface);
}*/

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID,
                                         const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize)
{
	USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;

	//uint8_t JoyStatus_LCL    = Joystick_GetStatus();
	//uint8_t ButtonStatus_LCL = Buttons_GetStatus();
	uint8_t UsedKeyCodes = 0;

	static int8_t test = 0;
	//if (JoyStatus_LCL & JOY_UP)
	if( btn_status & BTN_1  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_A;
	if( btn_status & BTN_2  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_B;
	if( btn_status & BTN_3  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_C;
	if( btn_status & BTN_4  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_D;
	if( btn_status & BTN_5  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_E;
	if( btn_status & BTN_6  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_F;
	if( btn_status & BTN_7  )
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_G;
	if( whl_status & WHL_1_CW )
	{
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_VOLUME_UP;
		whl_status &= ~WHL_1_CW;
	}
	if( whl_status & WHL_1_CCW )
	{
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_VOLUME_DOWN;
		whl_status &= ~WHL_1_CCW;
	}
	if( whl_status & WHL_2_CW )
	{
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_J;
		whl_status &= ~WHL_2_CW;
	}
	if( whl_status & WHL_2_CCW )
	{
		KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_K;
		whl_status &= ~WHL_2_CCW;
	}

	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the created report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	uint8_t  LEDMask   = 0x8;
	uint8_t* LEDReport = (uint8_t*)ReportData;

	if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK)
	  LEDMask |= 0x1;

	if (*LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
	  LEDMask |= 0x2;

	if (*LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
	  LEDMask |= 0x4;

	//LEDs_SetAllLEDs(LEDMask);*/
}

bool CALLBACK_RPC_Device_CreateRPCReport(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo,
										 uint8_t* const ReportID,
										 /*const uint8_t ReportType,*/
										 void* ReportData,
										 uint16_t* const ReportSize)
{
	USB_RPCReport_Data_t* RPCReport = (USB_RPCReport_Data_t*) ReportData;

	memcpy(RPCReport, &rpc_values, sizeof(USB_RPCReport_Data_t));
	/*RPCReport->v_water = 111;
	RPCReport->t_water = 40;
	RPCReport->v_fan   = 555;
	RPCReport->p_fan   = 55;
*/
	*ReportSize = sizeof(USB_RPCReport_Data_t);
	return false;
}

void CALLBACK_RPC_Device_ProcessRPCSetFanspeed(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo,
										  const uint8_t ReportID,
										  /*const uint8_t ReportType,*/
										  const void* ReportData,
										  const uint16_t ReportSize)
{
	// make sure its the data it's ment to be (single uint8_t value)
	if( ReportSize != 1 )
		return;
	fan_value = *((uint8_t*)ReportData);
	if( fan_value >= 0 && fan_value <= 100)
		fan_flow_status |= (1<<SET_FAN_FLAG);
	return;
}


void Frontpanel_Init()
{
	// Set input
	DDRB &= ~( (1<<PB4)|(1<<PB5) );
	DDRD &= ~( (1<<PD0)|(1<<PD1)|(1<<PD4)|(1<<PD5)|(1<<PD6)|(1<<PD7) );
	DDRF &= ~( (1<<PF4)|(1<<PF5)|(1<<PF6) );
	// Enable Pullups
	PORTB |= (1<<PB4) | (1<<PB5) ;
	PORTD |= (1<<PD0) | (1<<PD1) | (1<<PD4) | (1<<PD5) | (1<<PD6) | (1<<PD7) ;
	PORTF |= (1<<PF4) | (1<<PF5) | (1<<PF6) ;
	// Interrupts for Wheels
	//PCICR |= (1<<PCIE0);
	//PCMSK0 |= (1<<PCINT4) | (1<<PCINT5);
	// Timer0 (debounce) prescaler 256 = 256ms
	TCCR0B |= (1<<CS02) ;
	// Reset Timer0
	TCNT0 = 0;
}

void Waterflow_Init()
{
	// Set input
	DDRB &= ~(1<<PD2);
	// Enable INT2 external interrupt on rising edge
	EICRA |= (1<<ISC21) | (1<<ISC20);
	EIMSK |= (1<<INT2);
	// Enable Timer1 with prescaler 8
	TCCR1B |= (1<<CS11);
	// Timer1 overflow intterrupt
	TIMSK1 |= (1<<TOIE1);
	// Reset Timer
	TCNT1 = 0;
}

// Waterflow timer overflow -> Waterflow stopped
ISR(TIMER1_OVF_vect)
{
	rpc_values.v_water = 0;
}

// Waterflow interrupt found
ISR(INT2_vect)
{
	timer_flow = TCNT1;
	TCNT1 = 1;
	fan_flow_status |= (1<<INT_FLOW_FLAG);
}

// Calculate actual Waterflow value out of timer status
void Waterflow_Worker()
{
	uint32_t erg;
	// Check for new event
	if( fan_flow_status & (1<<INT_FLOW_FLAG) ) {
		erg = timer_flow;
		//l/h errechnen -> (18*F_OSZ)/(5*tmp_delta*Prescaler)
		erg = 3600000/(erg);
		if( !( ((uint16_t)erg<100) || ((uint16_t)erg>1000) ) )
			rpc_values.v_water = ((uint16_t)erg + 64*rpc_values.v_water)/65;
		fan_flow_status &= ~(1<<INT_FLOW_FLAG);
	}
}

void Watertemp_Init()
{
	// Set reference to internal 5V
	ADMUX |= (1<<REFS0) | (1<<MUX3) | (1<<MUX0);
	// Turn on ADC in auto conversion mode and enable interrupt
	ADCSRA |= (1<<ADEN) | (1<<ADSC) | (1<<ADATE) | (1<<ADIE) | (ADPS2) | (ADPS1) | (ADPS0);
}

ISR(ADC_vect)
{
	adc_voltage = ADC;
	// switching ADC off and on again is mandatory
	//ADCSRA &= ~(1<<ADEN);
	//ADCSRA |= (1<<ADEN);
	fan_flow_status |= (1<<INT_TEMP_FLAG);
}

void Watertemp_Worker()
{
	if( fan_flow_status & (1<<INT_TEMP_FLAG) ) {
		int16_t temp = (adc_voltage > 511) ? adc_voltage-1024 : adc_voltage;
		if( temp < -225 ) {
			temp = -10*temp+9900;
		} else if ( temp < 50 ) {
			temp = -12*temp+9450;
		} else if ( temp < 240 ) {
			temp = -15*temp+9600;
		} else {
			temp = -20*temp+10800;
		}
		rpc_values.t_water = (temp/30);
		fan_flow_status &= ~(1<<INT_TEMP_FLAG);
	}
}

void Fans_Init()
{
	// Set Input with pullup for fan tacho
	DDRD &= ~(1<<PD3);
	PORTD |= (1<<PD3);
	// Set Output for fans
	DDRE |= (1<<PE2);
	// test: PORTE |= (1<<PE2);
	// Enable external Interrupt INT3 on rising edge for capturing RPM FAN
	EICRA |= (1<<ISC31) | (1<<ISC30);
	EIMSK |= (1<<INT3);
	// Enable Timer3 with prescaler 64 for measuring RPM FAN
	TCCR3B |= (1<<CS11) | (1<<CS10);
	// Timer3 overflow intterrupt
	TIMSK3 |= (1<<TOIE3);
	// Reset Timer 3
	TCNT3 = 0;
	// Set Output for PWM
	DDRC |= (1<<PC7);
	// Init Timer4 for fast PWM Mode (clear on match)
	TCCR4A |= (1<<COM4A1) | (1<<PWM4A);
	// Set top to 1000
	TC4H = 0x0;
	OCR4C = 130;
	// Set prescaler to 2
	TCCR4B |= (1<<CS41);
	// Init PWM at 50%
	TC4H = 0x0;
	OCR4A = 35;
}

// Fan timer overflow
ISR(TIMER3_OVF_vect)
{
	rpc_values.v_fan = 0;
}

// rpm_fan Flanke erkannt
ISR(INT3_vect)
{
	timer_fan = TCNT3;
	TCNT3 = 0;
	fan_flow_status |= (1<<INT_FAN_FLAG);
}

void Fans_Worker(void)
{
	// Check for new event
	if( fan_flow_status & (1<<INT_FAN_FLAG) ) {
		uint32_t erg;
		erg = timer_fan;
		// r/m errechnen: (30*F_CPU)/(Prescaler * n)
		erg = 3750000/erg;//>>1); // /2, dadurch prescaler verdoppelt von 8 auf 16 (hier von 4 auf 8 wegen FCPU)
		if( !( ((uint16_t)erg<300) || ((uint16_t)erg>2000) ) )
			rpc_values.v_fan = ((uint16_t)erg + 32*rpc_values.v_fan)/33;
		fan_flow_status &= ~(1<<INT_FAN_FLAG);
	}
	// Check for new Fanspeed Set Event
	if( fan_flow_status & (1<<SET_FAN_FLAG) ) {
		TC4H = 0x0;
		OCR4A = 30+fan_value;
		if(fan_value == 0)
			PORTE &= ~(1<<PE2);
		else
			PORTE |= (1<<PE2);
		rpc_values.p_fan = fan_value;
		fan_flow_status &= ~(1<<SET_FAN_FLAG);
	}
	
}

void WriteButtonState(uint8_t *btn_mask)
{
	*btn_mask = 0;
	*btn_mask |= ((~PINF)&((1<<PF4)|(1<<PF5)|(1<<PF6))) >> 3;
	*btn_mask |= ((~PIND)&(1<<PD7)) >> 3;
	*btn_mask |= ((~PIND)&(1<<PD6)) >> 1;
	*btn_mask |= ((~PIND)&((1<<PD4)|(1<<PD5))) << 2;
}

void WriteWheelState(uint8_t *whl_mask)
{
	*whl_mask &= 0xf0;
	*whl_mask |= ((~PINB)&((1<<PB4)|(1<<PB5)));
	*whl_mask |= ((~PIND)&(1<<PD1)) << 5;
	*whl_mask |= ((~PIND)&(1<<PD0)) << 7;
}

void CheckButtonState(void)
{
	static uint8_t old_btn_status = 0;
	static uint8_t old_whl_status = 0;
	uint8_t new_btn_status = 0;
	uint8_t new_whl_status = 0;
	uint8_t saved_whl_status = 0;
	// check buttons on Timer0 overflow
	if( (TIFR0&(1<<TOV0)) != 0 )
	{
		WriteButtonState(&new_btn_status);
		WriteWheelState(&new_whl_status);
		if( new_btn_status == old_btn_status )
			btn_status = new_btn_status;
		if( new_whl_status == old_whl_status)
		{
			saved_whl_status = whl_status & 0xf0;
			whl_status &= 0x0f;
			whl_status |= new_whl_status & 0xf0;

			// Checking wheel 1
			if( !(saved_whl_status&WHL1A) && (whl_status&WHL1A) && !(whl_status&WHL1B) )
				whl_status |= WHL_1_CW;
			else if( (saved_whl_status&WHL1B) && !(whl_status&WHL1B) && !(whl_status&WHL1A) )
				whl_status |= WHL_1_CW;
			else if( (saved_whl_status&WHL1A) && !(whl_status&WHL1A) && !(whl_status&WHL1B) )
				whl_status |= WHL_1_CCW;
			else if( !(saved_whl_status&WHL1B) && (whl_status&WHL1B) && !(whl_status&WHL1A) )
				whl_status |= WHL_1_CCW;

			// Checking wheel 2
			if( !(saved_whl_status&WHL2A) && (whl_status&WHL2A) && !(whl_status&WHL2B) )
				whl_status |= WHL_2_CW;
			else if( (saved_whl_status&WHL2B) && !(whl_status&WHL2B) && !(whl_status&WHL2A) )
				whl_status |= WHL_2_CW;
			else if( (saved_whl_status&WHL2A) && !(whl_status&WHL2A) && !(whl_status&WHL2B) )
				whl_status |= WHL_2_CCW;
			else if( !(saved_whl_status&WHL2B) && (whl_status&WHL2B) && !(whl_status&WHL2A) )
				whl_status |= WHL_2_CCW;
		}
		old_btn_status = new_btn_status;
		old_whl_status = new_whl_status;
		// Clear overflow
		TIFR0 |= (1<<TOV0);

		if( saved_whl_status == (whl_status&0xF0) )
			return;

	}
}

/*ISR(PCINT0_vect)
{
	static uint8_t pinb_old = 0xFF;
	if( pinb_old == PINB )
		return;
	// Clockwise rotation
	if( (pinb_old&(1<<PB4))==0 && (PINB&(1<<PB4))!=0 && (PINB&(1<<PB5))==0 )
		whl_status |= WHL_1_CW;
	// Couterclockwise rotation
	if( (pinb_old&(1<<PB5))==0 && (PINB&(1<<PB5))!=0 && (PINB&(1<<PB4))==0 )
		whl_status |= WHL_1_CCW;
	pinb_old = PINB;
}*/

