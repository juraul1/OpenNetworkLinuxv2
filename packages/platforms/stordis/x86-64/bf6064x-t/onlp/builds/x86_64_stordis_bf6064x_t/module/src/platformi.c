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
#include "dpapi/dpapi_init.h"
#include "dpapi/dpapi_sfp.h"

#include "x86_64_stordis_bf6064x_t_int.h"
#include "x86_64_stordis_bf6064x_t_log.h"


const char*
onlp_platformi_get(void)
{
    return "x86-64-stordis-bf6064x-t-r0";
}

int
onlp_platformi_sw_init(void)
{
    int port;

    if(dpapi_init() != ONLP_STATUS_OK){
        printf("dpapi_init failed!");
    }

    for(port = 0; port < 64; port++)
    {
        if(dpapi_sfp_control_set(port, DPAPI_SFP_CONTROL_LP_MODE, 0) != ONLP_STATUS_OK)
        {
            return ONLP_STATUS_E_INVALID;
        }
        if(dpapi_sfp_control_set(port, DPAPI_SFP_CONTROL_RESET, 0) != ONLP_STATUS_OK)
        {
            return ONLP_STATUS_E_INVALID;
        }
    }
    
    return 0;
}

int
onlp_platformi_manage_fans(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_platformi_manage_leds(void)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}
