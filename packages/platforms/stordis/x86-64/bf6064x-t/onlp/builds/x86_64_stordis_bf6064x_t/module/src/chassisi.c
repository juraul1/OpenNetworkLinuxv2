#include <onlp/platformi/chassisi.h>
#include "platform_lib.h"

int
onlp_chassisi_hdr_get(onlp_oid_id_t id, onlp_oid_hdr_t* hdr)
{
    int i;
    onlp_oid_t* e = hdr->coids;

    ONLP_OID_STATUS_FLAG_SET(hdr, PRESENT);
    ONLP_OID_STATUS_FLAG_SET(hdr, OPERATIONAL);

    /* 2 PSUs on the chassis */
    for (i = 1; i <= 2; i++)
    {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* 4 SYSLEDs on the chassis */
    for (i = 1; i <= 4; i++)
    {
        *e++ = ONLP_LED_ID_CREATE(i);
    }

    /* 10 Fans on the chassis */
    for (i = 1; i <= 10; i++)
    {
        *e++ = ONLP_FAN_ID_CREATE(i);
    }
    
    /* 6 Thermal sensors on the chassis */
    for (i = 1; i <= 6; i++)
    {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* 64 QSFPs */
    for(i = 1; i <= 64; i++) {
        *e++ = ONLP_SFP_ID_CREATE(i);
    }
    return 0;
}

int
onlp_chassisi_info_get(onlp_oid_id_t id, onlp_chassis_info_t* info)
{
    return onlp_chassisi_hdr_get(id, &info->hdr);
}
