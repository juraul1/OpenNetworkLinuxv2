#include <string.h>
#include <onlp/platformi/attributei.h>
#include <onlp/stdattrs.h>
#include <onlplib/file.h>

#include "platform_lib.h"
#include "dpapi/dpapi.h"
#include "dpapi/dpapi_sys.h"

int
onlp_attributei_onie_info_get(onlp_oid_t oid, onlp_onie_info_t* rp)
{
    if(oid != ONLP_OID_CHASSIS) {
	    return ONLP_STATUS_E_UNSUPPORTED;
    }

    int id = ONLP_OID_ID_GET(oid);

    dpapi_onie_info_t onie_info;
    if(dpapi_sys_onie_info_get(id-1, &onie_info) != ONLP_STATUS_OK)
    {
        return ONLP_STATUS_E_INVALID;
    }

    memcpy(rp, &onie_info, sizeof(onlp_onie_info_t));

    return 0;
    //return ONLP_STATUS_E_UNSUPPORTED;
}

int
onlp_attributei_asset_info_get(onlp_oid_t oid, onlp_asset_info_t* rp)
{
    if(oid != ONLP_OID_CHASSIS) {
	    return ONLP_STATUS_E_UNSUPPORTED;
    }

    return ONLP_STATUS_E_UNSUPPORTED;
}
