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
#include "dpapi/dpapi_sfp.h"

#include "lock.h"


int
onlp_sfpi_type_get(onlp_oid_id_t oid, onlp_sfp_type_t* rtype)
{
    uint8_t buffer[256];
    int id = ONLP_OID_ID_GET(oid);

    bf6064x_lock_acquire();

    uint8_t present = 0;
    if (dpapi_sfp_is_present(id, &present) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    if (!present) 
    {
        return ONLP_STATUS_E_MISSING;
    }

    if(dpapi_sfp_eeprom_read(id, buffer) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    bf6064x_lock_release();

    sff_eeprom_t eeprom;
    memset(&eeprom, 0, sizeof(eeprom));

    if (sff_eeprom_parse(&eeprom, buffer) != 0)
    {
        return ONLP_STATUS_E_INVALID;
    }
    *rtype = eeprom.info.sfp_type;
    return 0;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    /*
     * Ports {0, 64}
     */
    uint8_t p, max_ports;

    bf6064x_lock_acquire();

    int rv = dpapi_sfp_quantity_of_ports_get(&max_ports);

    bf6064x_lock_release();

    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }


    AIM_BITMAP_CLR_ALL(bmap);

    for(p = 0; p < max_ports; p++) 
    {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(onlp_oid_id_t oid)
{
    /*
     * Return 1 if present.
     * Return 0 if not present.
     * Return < 0 if error.
     */
    int id = ONLP_OID_ID_GET(oid);
    uint8_t present = 0;

    bf6064x_lock_acquire();

    dpapi_sfp_is_present(id, &present);

    bf6064x_lock_release();

    return present;
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    uint8_t max_ports, index, *pbmap;

    int rv;

    bf6064x_lock_acquire();

    rv = dpapi_sfp_quantity_of_ports_get(&max_ports);
    
    bf6064x_lock_release();


    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    pbmap = (uint8_t *)calloc(1, sizeof(uint8_t) * max_ports);

    bf6064x_lock_acquire();

    rv = dpapi_sfp_presence_bitmap_get(pbmap);

    bf6064x_lock_release();

    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }


    for(index = 0; index<max_ports; index++)
    {
        AIM_BITMAP_MOD(dst, index, pbmap[index]);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_read(onlp_oid_id_t oid, int devaddr, int addr,
                   uint8_t* dst, int size)
{
    int id = ONLP_OID_ID_GET(oid);

    bf6064x_lock_acquire();

    int rv = dpapi_sfp_eeprom_read(id, dst);
    
    bf6064x_lock_release();


    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_dev_readb(onlp_oid_id_t oid, int devaddr, int addr)
{
    int id = ONLP_OID_ID_GET(oid);
    uint8_t data = 0;

    bf6064x_lock_acquire();

    int rv = dpapi_sfp_eeprom_readb(id, addr, &data);
    
    bf6064x_lock_release();

    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }


    return data;
}

int
onlp_sfpi_dev_writeb(onlp_oid_id_t oid, int devaddr, int addr, uint8_t value)
{
    int id = ONLP_OID_ID_GET(oid);

    bf6064x_lock_acquire();

    int rv = dpapi_sfp_eeprom_writeb(id, addr, value);

    bf6064x_lock_release();
 
    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }
    

    return ONLP_STATUS_OK;
}
/*
int onlp_sfpi_control_supported(onlp_oid_id_t oid, onlp_sfp_control_t control, int* rv)
{
    int id = ONLP_OID_ID_GET(oid);
    if(dpapi_sfp_control_is_support(id, control, (uint8_t *)rv) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    return ONLP_STATUS_OK;
}

int onlp_sfpi_control_get(onlp_oid_id_t oid, onlp_sfp_control_t control, int* value)
{
    int id = ONLP_OID_ID_GET(oid);
    if(dpapi_sfp_control_get(id, control, (uint8_t *)value) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    return ONLP_STATUS_OK;
}

int onlp_sfpi_control_set(onlp_oid_id_t oid, onlp_sfp_control_t control, int value)
{
    int id = ONLP_OID_ID_GET(oid);
    if(dpapi_sfp_control_set(id, control, value) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    return ONLP_STATUS_OK;
}
*/
int onlp_sfpi_rx_los_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    uint8_t max_ports, index, *pbmap;
    int rv;

    bf6064x_lock_acquire();

    rv = dpapi_sfp_quantity_of_ports_get(&max_ports);

    bf6064x_lock_release();

    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    pbmap = (uint8_t *)calloc(1, sizeof(uint8_t) * max_ports);
    
    bf6064x_lock_acquire();

    rv = dpapi_sfp_rx_los_bitmap_get(pbmap);

    bf6064x_lock_release();

    if(rv != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }
    
    for(index = 0; index<max_ports; index++) 
    {
        AIM_BITMAP_MOD(dst, index, pbmap[index]);
    }

    return ONLP_STATUS_OK;
}
