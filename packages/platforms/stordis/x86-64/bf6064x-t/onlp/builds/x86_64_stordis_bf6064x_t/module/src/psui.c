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
 *
 *
 ***********************************************************/
#include <onlp/platformi/base.h>
#include "platform_lib.h"
#include "dpapi/dpapi.h"
#include "dpapi/dpapi_psu.h"

#define PSU_INFO_ENTRY_INIT(_id, _desc)    \
    {                                      \
        {                                  \
            .id = ONLP_PSU_ID_CREATE(_id), \
            .description = _desc,          \
            .poid = ONLP_OID_CHASSIS,      \
        },                                 \
        .caps = (ONLP_PSU_CAPS_GET_VOUT | ONLP_PSU_CAPS_GET_IOUT | ONLP_PSU_CAPS_GET_POUT), \
        .type = ONLP_PSU_TYPE_DC12, \
    }

enum onlp_psu_id
{
    PSU_RESERVED = 0,
    PSU_1,
    PSU_2,
};

static onlp_psu_info_t onlp_psu_info[] = {
    { }, /* Not used */
    PSU_INFO_ENTRY_INIT(PSU_1, "PSU 1"),
    PSU_INFO_ENTRY_INIT(PSU_2, "PSU 2"),
};


int
onlp_psui_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t* hdr)
{
    int id = ONLP_OID_ID_GET(oid);
    uint8_t present = 0;
    *hdr = onlp_psu_info[id].hdr;

    if(dpapi_psu_is_present(id-1, &present) != ONLP_STATUS_OK)
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
onlp_psui_info_get(onlp_oid_t oid, onlp_psu_info_t* info)
{
    int id = ONLP_OID_ID_GET(oid);
    char *model, *serial;
    int mvout = 0, miout = 0, mpout = 0;
    uint8_t present = 0;

    ONLP_OID_INFO_ASSIGN(id, onlp_psu_info, info);

    if(dpapi_psu_is_present(id-1, &present) != ONLP_STATUS_OK)
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

    if(dpapi_psu_model_get(id-1, &model) == ONLP_STATUS_OK)
    {
        strcpy(info->model, model);
    }

    if(dpapi_psu_serial_get(id-1, &serial) == ONLP_STATUS_OK)
    {
        strcpy(info->serial, serial);
    }

    if(dpapi_psu_volt_get(id-1,  &mvout) == ONLP_STATUS_OK)
    {
        info->mvout = mvout;
    }

    if(dpapi_psu_amp_get(id-1,  &miout) == ONLP_STATUS_OK)
    {
        info->miout = miout;
    }

    if(dpapi_psu_watt_get(id-1,  &mpout) == ONLP_STATUS_OK)
    {
        info->mpout = mpout;
    }
    
    return ONLP_STATUS_OK;
}
