#ifndef __DPAPI_H__
#define __DPAPI_H__

#include <stdint.h>
typedef int dpapi_status;

typedef enum
{
    DPAPI_LOG_TRACE, 
    DPAPI_LOG_DEBUG, 
    DPAPI_LOG_INFO, 
    DPAPI_LOG_WARN, 
    DPAPI_LOG_ERROR, 
    DPAPI_LOG_FATAL 
} dpapi_log_level_t;

/* Compatible with ONLP OID TYPE */
typedef enum
{
    DPAPI_TYPE_SYS = 1,
    DPAPI_TYPE_THERMAL,
    DPAPI_TYPE_FAN,
    DPAPI_TYPE_PSU,
    DPAPI_TYPE_SYSLED,
    DPAPI_TYPE_SFP
} dpapi_element_type_t;

dpapi_status dpapi_log_level_set(dpapi_log_level_t level);
dpapi_status dpapi_log_quiet_set(uint8_t enable);
dpapi_status dpapi_nof_resource_in_type_get(
    dpapi_element_type_t type, uint16_t *quantity);

#endif /* __DPAPI_H__ */