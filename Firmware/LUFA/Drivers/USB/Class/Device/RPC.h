#ifndef _RPC_CLASS_DEVICE_H_
#define _RPC_CLASS_DEVICE_H_

	/* Includes: */
		#include "../../USB.h"
		/*#include "../Common/HID.h"*/

	/* Enable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			extern "C" {
		#endif

	/* Preprocessor Checks: */
		#if !defined(__INCLUDE_FROM_RPC_DRIVER)
			#error Do not include this file directly. Include LUFA/Drivers/USB.h instead.
		#endif

	/* Public Interface - May be used in end-application: */
		/* Type Defines: */
				
			enum RPC_ClassRequests_t
			{
				//RPC_REQ_GetReport       = 0x01, /**< HID class-specific Request to get the current HID report from the device. */
				//RPC_REQ_GetIdle         = 0x02, /**< HID class-specific Request to get the current device idle count. */
				//RPC_REQ_GetProtocol     = 0x03, /**< HID class-specific Request to get the current HID report protocol mode. */
				RPC_REQ_SetReport       = 0x09, /**< HID class-specific Request to set the current HID report to the device. */
				//RPC_REQ_SetIdle         = 0x0A, /**< HID class-specific Request to set the device's idle count. */
				//RPC_REQ_SetProtocol     = 0x0B, /**< HID class-specific Request to set the current HID report protocol mode. */
			};
				
			typedef struct
			{
				const struct
				{
					uint8_t  InterfaceNumber; /**< Interface number of the RPC interface within the device. */

					uint8_t  ReportINEndpointNumber; /**< Endpoint number of the RPC interface's IN report endpoint. */
					uint8_t  ReportINEndpointSize; /**< Size in bytes of the RPC interface's IN report endpoint. */
					bool     ReportINEndpointDoubleBank; /**< Indicates if the RPC interface's IN report endpoint should use double banking. */
					
					//uint8_t  ReportOUTEndpointNumber; /**< Endpoint number of the RPC interface's IN report endpoint. */
					//uint8_t  ReportOUTEndpointSize; /**< Size in bytes of the RPC interface's IN report endpoint. */
					//bool     ReportOUTEndpointDoubleBank; /**< Indicates if the RPC interface's IN report endpoint should use double banking. */
				} Config; /**< Config data for the USB class interface within the device. All elements in this section
				           *   <b>must</b> be set or the interface will fail to enumerate and operate correctly.
				           */
				//struct
				//{
				//	bool     UsingReportProtocol; /**< Indicates if the RPC interface is set to Boot or Report protocol mode. */
				//	uint16_t IdleCount; /**< Report idle period, in milliseconds, set by the host. */
				//	uint16_t IdleMSRemaining; /**< Total number of milliseconds remaining before the idle period elapsed - this
				//							   *   should be decremented by the user application if non-zero each millisecond. */
				//} State; /**< State data for the USB class interface within the device. All elements in this section
				//          *   are reset to their defaults when the interface is enumerated.
				//          */
			} USB_ClassInfo_RPC_Device_t;

		/* Function Prototypes: */
			/** Configures the endpoints of a given HID interface, ready for use. This should be linked to the library
			 *  \ref EVENT_USB_Device_ConfigurationChanged() event so that the endpoints are configured when the configuration
			 *  containing the given HID interface is selected.
			 *
			 *  \note The endpoint index numbers as given in the interface's configuration structure must not overlap with any other
			 *        interface, or endpoint bank corruption will occur. Gaps in the allocated endpoint numbers or non-sequential indexes
			 *        within a single interface is allowed, but no two interfaces of any type have have interleaved endpoint indexes.
			 *
			 *  \param[in,out] HIDInterfaceInfo  Pointer to a structure containing a HID Class configuration and state.
			 *
			 *  \return Boolean \c true if the endpoints were successfully configured, \c false otherwise.
			 */
			bool RPC_Device_ConfigureEndpoints(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo) ATTR_NON_NULL_PTR_ARG(1);

			/** Processes incoming control requests from the host, that are directed to the given HID class interface. This should be
			 *  linked to the library \ref EVENT_USB_Device_ControlRequest() event.
			 *
			 *  \param[in,out] HIDInterfaceInfo  Pointer to a structure containing a HID Class configuration and state.
			 */
			void RPC_Device_ProcessControlRequest(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo) ATTR_NON_NULL_PTR_ARG(1);

			/** General management task for a given HID class interface, required for the correct operation of the interface. This should
			 *  be called frequently in the main program loop, before the master USB management task \ref USB_USBTask().
			 *
			 *  \param[in,out] HIDInterfaceInfo  Pointer to a structure containing a HID Class configuration and state.
			 */
			void RPC_Device_USBTask(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo) ATTR_NON_NULL_PTR_ARG(1);

			/** HID class driver callback for the user creation of a HID IN report. This callback may fire in response to either
			 *  HID class control requests from the host, or by the normal HID endpoint polling procedure. Inside this callback the
			 *  user is responsible for the creation of the next HID input report to be sent to the host.
			 *
			 *  \param[in,out] HIDInterfaceInfo  Pointer to a structure containing a HID Class configuration and state.
			 *  \param[in,out] ReportID          If preset to a non-zero value, this is the report ID being requested by the host. If zero,
			 *                                   this should be set to the report ID of the generated HID input report (if any). If multiple
			 *                                   reports are not sent via the given HID interface, this parameter should be ignored.
			 *  \param[in]     ReportType        Type of HID report to generate, either \ref HID_REPORT_ITEM_In or \ref HID_REPORT_ITEM_Feature.
			 *  \param[out]    ReportData        Pointer to a buffer where the generated HID report should be stored.
			 *  \param[out]    ReportSize        Number of bytes in the generated input report, or zero if no report is to be sent.
			 *
			 *  \return Boolean \c true to force the sending of the report even if it is identical to the previous report and still within
			 *          the idle period (useful for devices which report relative movement), \c false otherwise.
			 */
			bool CALLBACK_RPC_Device_CreateRPCReport(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo,
			                                         uint8_t* const ReportID,
			                                         /*const uint8_t ReportType,*/
			                                         void* ReportData,
			                                         uint16_t* const ReportSize) ATTR_NON_NULL_PTR_ARG(1)
			                                         ATTR_NON_NULL_PTR_ARG(2) ATTR_NON_NULL_PTR_ARG(3) ATTR_NON_NULL_PTR_ARG(4);

			/** HID class driver callback for the user processing of a received HID OUT report. This callback may fire in response to
			 *  either HID class control requests from the host, or by the normal HID endpoint polling procedure. Inside this callback
			 *  the user is responsible for the processing of the received HID output report from the host.
			 *
			 *  \param[in,out] HIDInterfaceInfo  Pointer to a structure containing a HID Class configuration and state.
			 *  \param[in]     ReportID          Report ID of the received output report. If multiple reports are not received via the given HID
			 *                                   interface, this parameter should be ignored.
			 *  \param[in]     ReportType        Type of received HID report, either \ref HID_REPORT_ITEM_Out or \ref HID_REPORT_ITEM_Feature.
			 *  \param[in]     ReportData        Pointer to a buffer where the received HID report is stored.
			 *  \param[in]     ReportSize        Size in bytes of  the received report from the host.
			 */
			void CALLBACK_RPC_Device_ProcessRPCReport(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo,
			                                          const uint8_t ReportID,
			                                          /*const uint8_t ReportType,*/
			                                          const void* ReportData,
			                                          const uint16_t ReportSize) ATTR_NON_NULL_PTR_ARG(1) ATTR_NON_NULL_PTR_ARG(3);

		/* Inline Functions: */
			/** Indicates that a millisecond of idle time has elapsed on the given HID interface, and the interface's idle count should be
			 *  decremented. This should be called once per millisecond so that hardware key-repeats function correctly. It is recommended
			 *  that this be called by the \ref EVENT_USB_Device_StartOfFrame() event, once SOF events have been enabled via
			 *  \ref USB_Device_EnableSOFEvents().
			 *
			 *  \param[in,out] HIDInterfaceInfo  Pointer to a structure containing a HID Class configuration and state.
			 */
			//static inline void RPC_Device_MillisecondElapsed(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo) ATTR_ALWAYS_INLINE ATTR_NON_NULL_PTR_ARG(1);
			//static inline void RPC_Device_MillisecondElapsed(USB_ClassInfo_RPC_Device_t* const RPCInterfaceInfo)
			//{
			//	if (RPCInterfaceInfo->State.IdleMSRemaining)
			//	  RPCInterfaceInfo->State.IdleMSRemaining--;
			//}

	/* Disable C linkage for C++ Compilers: */
		#if defined(__cplusplus)
			}
		#endif

#endif

/** @} */

