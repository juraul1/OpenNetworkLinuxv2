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
#include "dpapi/dpapi_sysled.h"

enum onlp_led_id
{
    LED_RESERVED = 0,
    LED1,
    LED2,
    LED3,
    LED4,
};

#define LED_INFO_ENTRY_INIT(_id, _desc, _caps, _defaultm)      \
    {                                               \
        {                                           \
            .id = ONLP_LED_ID_CREATE(_id),          \
            .description = _desc,                   \
            .poid = ONLP_OID_CHASSIS,               \
            .status = ONLP_OID_STATUS_FLAG_PRESENT, \
        },                                          \
        .caps = _caps,                              \
        .mode = _defaultm,                          \
    }

static onlp_led_info_t onlp_led_info[] =
{
    { }, /* Not used */
    LED_INFO_ENTRY_INIT(LED1, "sysled-fan", 
        (ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN ), ONLP_LED_MODE_GREEN),
    LED_INFO_ENTRY_INIT(LED2, "sysled-system", 
        (ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_ORANGE_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING ), ONLP_LED_MODE_GREEN_BLINKING),
    LED_INFO_ENTRY_INIT(LED3, "sysled-power-0", 
        (ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN ), ONLP_LED_MODE_GREEN),
    LED_INFO_ENTRY_INIT(LED4, "sysled-power-1", 
        (ONLP_LED_CAPS_OFF | ONLP_LED_CAPS_ORANGE | ONLP_LED_CAPS_GREEN ), ONLP_LED_MODE_GREEN),
};

int
onlp_ledi_hdr_get(onlp_oid_t oid, onlp_oid_hdr_t* hdr)
{
    int id = ONLP_OID_ID_GET(oid);
    *hdr = onlp_led_info[id].hdr;
    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t oid, onlp_led_info_t* info)
{
    int id = ONLP_OID_ID_GET(oid);
    ONLP_OID_INFO_ASSIGN(id, onlp_led_info, info);

    if(dpapi_sysled_mode_get(id-1, (dpapi_led_mode_t *) &info->mode) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    return ONLP_STATUS_OK;
}

int
onlp_ledi_mode_set(onlp_oid_t oid, onlp_led_mode_t mode)
{
    int id = ONLP_OID_ID_GET(oid);

    if (dpapi_sysled_mode_set(id-1, mode) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}
