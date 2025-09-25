//Generate at 2025-05-23 10:26:56 by E:\MCU_BLEKEY\ALL_Project\CANbus\dtc_cdd(0324)\canBusDemo.cdd


/******************************************************************************

                  Copyright @1998 - 2023 YF.TECH

 ******************************************************************************				
@file Dem_Cfg.h
@brief Configuration for DTCs\Extend Data\Snapshot in CDD file
@author LGY

******************************************************************************/
#ifndef DEM_CFG_H_
#define DEM_CFG_H_

#define DEM_DEV_ERROR_DETECT				STD_ON		// Activate/Deactivate Dev Error Detection and Notification.
#define DEM_USE_PRIMARY_MEMORY_SUPPORT      STD_ON		// Activate/Deactivate Primary Memory.
#define DEM_CLEAR_ALL_EVENTS				STD_OFF		// All event or only events with DTC is cleared with Dem_ClearDTC
#define DEM_ENABLE_OPRATION_CYCLE_COUNTER	STD_ON		// Activate/Deactivate operation Cycle  counter
#define DEM_TYPE_OF_DTC_SUPPORTED			DEM_ISO15031_6         // ISO15031-6DTCFormat

#define DEM_ENABLE_CONDITION_SUPPORT	    STD_ON		// Activate/Deactivate enable conditon
#define DEM_ENABLE_TIMEBASE_SUPPORT	        STD_OFF		// Activate/Deactivate enable time base
#define DEM_DTC_STATUS_AVAILABILITY_MASK	0xFF           // defined by vehicle manufacture
#define DEM_MAX_NR_OF_RECORDS_IN_EXTENDED_DATA	 4	// 0..253 according to Autosar
#define DEM_MAX_NR_OF_EVENT_DESTINATION          1	// 0..4 according to Autosar


#define DEM_MAX_NR_OF_CLASSES_IN_FREEZEFRAME_DATA  1 //Max number of snapshot record
#define DEM_MAX_NR_OF_RECORDS_IN_FREEZEFRAME_DATA  9 //Max number of DID in one freeze frame

#define DEM_MAX_SIZE_FF_DATA					(19)	// Max number of bytes in one freeze frame
#define DEM_MAX_SIZE_EXT_DATA					0	//10 Max number of bytes in one extended data record

/*----------------------------------------------*
 * ���Ͷ���                            *
 *----------------------------------------------*/


/*----------------------------------------------*
 * �ⲿ��������                                     *
 *----------------------------------------------*/


/*----------------------------------------------*
 * ����ԭ��˵��                                   *
 *----------------------------------------------*/



#endif /*DEM_CFG_H_*/

