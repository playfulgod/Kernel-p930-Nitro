/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      User Data Backup File
PROJECT
   VN251

GENERAL DESCRIPTION
   

Copyright (c) 2010 LG Electronics. All right reserved.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*============================================================================

                      EDIT HISTORY FOR FILE

 This section contains comments describing changes made to this file.
 Notice that changes are listed in reverse chronological order.

 when      who    what, where, why
 --------  -----  ----------------------------------------------------------
 10/04/09  bkshin Initial Release
============================================================================*/
#ifndef _USER_DATA_BACK_UP_DIAG_H_
#define _USER_DATA_BACK_UP_DIAG_H_


//#include "comdef.h"

/*===========================================================================
                      MACRO DECLARATIONS
===========================================================================*/

/*===========================================================================
                      TYPE DEFINITIONS
===========================================================================*/
typedef enum
{
	UDBU_ERROR_SUCCESS = 0,
	UDBU_ERROR_CANNOT_COMPLETE =1,
	UDBU_ERROR_BAD_COMMAND =2,
	UDBU_ERROR_CRC =3,
	UDBU_ERROR_BAD_LENGTH=4,
	UDBU_ERROR_NOT_SUPPORTED=5,
	UDBU_ERROR_INVALID_PARAMETER=6,
	UDBU_ERROR_INVALID_ENTRY=7,
	UDBU_ERROR_VERSION_PARSE_ERROR=8
} udbu_error_type;

typedef enum
{
	USERDATA_BACKUP_REQUEST =	0,
	GET_DOWNLOAD_INFO       =	1,
	EXTRA_NV_OPERATION      =   2,
	PRL_OPERATION           =   3,
	SRD_INIT_OPERATION	    =   4,
	USERDATA_BACKUP_REQUEST_MDM =	5,
	USERDATA_RESTORE_REQUEST =	6,
	MAX_SUB_COMMAND
}udbu_sub_cmd_type;

#ifdef LG_FW_SRD_EXTRA_NV
typedef enum
{
	EXTRANV_BACKUP = 1,
	EXTRANV_INIT   = 2,
	EXTRANV_DELETE = 3,
	EXTRANV_BACKUP_END = 4
}udbu_extra_cmd_type;

#endif

//CSFB SRD
typedef enum
{
	NV_UPGRADE=0,
	NV_BINDL=1,
	NV_SKIP=2,
	NV_REFURBISH=3,
	NV_END=4,
} nv_item_info_type;

typedef enum
{
	UPGRADE=1,
	BINDL=2,
	REFURBISH=3,
	BOARDDL=4,
	BOARDDLINHUNTSVILLE=5,
} nv_item_dl_type;

typedef enum
{
	RECOVERY_FAIL=9,
    BACKUP_USE_DONE=10, 
} srd_recovery_result_type;

typedef struct
{
	unsigned short nv_enum;
	int nv_nam;
	int nam_cnt;
	unsigned int nv_type;  //byte
} xml_parser_nv_item_info;

typedef struct
{
	unsigned char item[128];
} nv_item_type;

#define NV_BACK_UP_LIST_START	0
#define NV_BACK_UP_LIST_END	65535

#ifndef LGNV_START
#define LGNV_START          0x8000  	/* LGNV ���۹��� */
typedef enum {
	NV_LG_FW_START_I = LGNV_START|(1/*LGNV_ENUM_PART_FW*/<<10), /* 33792 */

	NV_LG_FW_LAUNCH_KIT_I,
	NV_LG_FW_OTA_CHECK_CLNR_I,
	NV_LG_FW_CALL_LIFETIMER_I,
	NV_LG_FW_CALL_WDC_I,
	NV_LG_FW_SRD_DONE_I,
	NV_LG_FW_MAX_I,
} lgnv_FW_items_enum_type;
#endif

#endif
