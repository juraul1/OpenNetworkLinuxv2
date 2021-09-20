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
 ************************************************************
 *
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlp/platformi/base.h>
#include "platform_lib.h"
#include "dpapi/dpapi.h"
#include "dpapi/dpapi_thrml.h"

#include "lock.h"

#define THERMAL_INFO_ENTRY_INIT(_id, _desc)         \
    {                                               \
        {                                           \
            .id = ONLP_THERMAL_ID_CREATE(_id),      \
            .description = _desc,                   \
            .poid = ONLP_OID_CHASSIS,               \
            .status = ONLP_OID_STATUS_FLAG_PRESENT, \
        },                                          \
        .caps = (ONLP_THERMAL_CAPS_GET_TEMPERATURE | ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD), \
    }

enum onlp_thermal_id
{
    THERMAL_RESERVED = 0,
    THERMAL_1_MAC_1,
    THERMAL_2_MAC_2,
    THERMAL_3_INLET_1,
    THERMAL_4_INLET_2,
    THERMAL_5_INLET_3,
    THERMAL_6_OUTLET_1
};

static onlp_thermal_info_t onlp_thermal_info[] = {
    { }, /* Not used */
    THERMAL_INFO_ENTRY_INIT(THERMAL_1_MAC_1, "MAC Thermal Sensor 1"),
    THERMAL_INFO_ENTRY_INIT(THERMAL_2_MAC_2, "MAC Thermal Sensor 2"),
    THERMAL_INFO_ENTRY_INIT(THERMAL_3_INLET_1, "INLET Thermal Sensor 1"),
    THERMAL_INFO_ENTRY_INIT(THERMAL_4_INLET_2, "INLET Thermal Sensor 2"),
    THERMAL_INFO_ENTRY_INIT(THERMAL_5_INLET_3, "INLET Thermal Sensor 3"),
    THERMAL_INFO_ENTRY_INIT(THERMAL_6_OUTLET_1, "OUTLET Thermal Sensor 1"),
};

int
onlp_thermali_hdr_get(onlp_oid_id_t oid, onlp_oid_hdr_t* hdr)
{
    int id = ONLP_OID_ID_GET(oid);

    *hdr = onlp_thermal_info[id].hdr;
    return ONLP_STATUS_OK;
}

int
onlp_thermali_info_get(onlp_oid_id_t oid, onlp_thermal_info_t* info)
{
    int id = ONLP_OID_ID_GET(oid);
    ONLP_OID_INFO_ASSIGN(id, onlp_thermal_info, info);
    /* ONLPv2 is 1 base, but DAPAI is 0 base */

    bf6064x_lock_acquire();

    int rv = dpapi_thrml_temp_get(id-1, &info->mcelsius);

    bf6064x_lock_release();

    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

