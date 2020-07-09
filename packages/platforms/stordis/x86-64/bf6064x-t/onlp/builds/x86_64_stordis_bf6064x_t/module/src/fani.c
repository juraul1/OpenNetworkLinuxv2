/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************/
#include <onlp/platformi/base.h>
#include "platform_lib.h"
#include "dpapi/dpapi.h"
#include "dpapi/dpapi_fan.h"

enum onlp_fan_id
{
    FAN_RESERVED = 0,
    FAN1_IN_TRAY_1,
    FAN2_IN_TRAY_1,
    FAN3_IN_TRAY_1,
    FAN4_IN_TRAY_1,
    FAN5_IN_TRAY_2,
    FAN6_IN_TRAY_2,
    FAN7_IN_TRAY_2,
    FAN8_IN_TRAY_2,
    FAN9_PSU1,
    FAN10_PSU2
};

#define FAN_INFO_ENTRY_INIT(_id, _desc, _caps)    \
    {                                      \
        {                                  \
            .id = ONLP_FAN_ID_CREATE(_id), \
            .description = _desc,          \
            .poid = ONLP_OID_CHASSIS,      \
        },                                 \
        .dir = ONLP_FAN_DIR_UNKNOWN,       \
        .caps = _caps,                     \
    }


/* Static fan information */
onlp_fan_info_t onlp_fan_info[] = {
    { }, /* Not used */
    FAN_INFO_ENTRY_INIT(FAN1_IN_TRAY_1, "Chassis Fan 1-1", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN2_IN_TRAY_1, "Chassis Fan 1-2", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN3_IN_TRAY_1, "Chassis Fan 1-3", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN4_IN_TRAY_1, "Chassis Fan 1-4", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN5_IN_TRAY_2, "Chassis Fan 2-1", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN6_IN_TRAY_2, "Chassis Fan 2-2", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN7_IN_TRAY_2, "Chassis Fan 2-3", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN8_IN_TRAY_2, "Chassis Fan 2-4", 
        (ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN9_PSU1,      "Chassis PSU0-Fan", 
        (ONLP_FAN_CAPS_GET_PERCENTAGE)),
    FAN_INFO_ENTRY_INIT(FAN10_PSU2,     "Chassis PSU1-Fan", 
        (ONLP_FAN_CAPS_GET_PERCENTAGE))
};

int
onlp_fani_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t* hdr)
{
    int id = ONLP_OID_ID_GET(oid);
    uint8_t present = 0;
    *hdr = onlp_fan_info[id].hdr;
    
    if(dpapi_fan_is_present(id-1, &present) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    if(present){
        ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    }
    else
    {
        ONLP_OID_STATUS_FLAG_SET(hdr, FAILED);
    }

    return ONLP_STATUS_OK;
}


int
onlp_fani_info_get(onlp_oid_t oid, onlp_fan_info_t* info)
{
    int id = ONLP_OID_ID_GET(oid);
    uint8_t present = 0;
    ONLP_OID_INFO_ASSIGN(id, onlp_fan_info, info);

    if(dpapi_fan_is_present(id-1, &present) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    if(present){
        ONLP_OID_STATUS_FLAG_SET(info, PRESENT);
    }
    else
    {
        ONLP_OID_STATUS_FLAG_SET(info, FAILED);
        return ONLP_STATUS_OK;
    }

    if(dpapi_fan_percentage_get(id-1, (uint16_t *)&info->percentage) != ONLP_STATUS_OK)
    {
        info->percentage = 0;
    }

    return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t oid, int p)
{
    int id = ONLP_OID_ID_GET(oid);

    if(dpapi_fan_percentage_set(id-1, (uint16_t)p) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}
