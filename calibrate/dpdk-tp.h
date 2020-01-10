#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER dpdk

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./dpdk-tp.h"

#if !defined(_DPDK_TP_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define _DPDK_TP_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    dpdk,
    zero_arg,
    TP_ARGS(void),
    TP_FIELDS()
)

#endif /* _DPDK_TP_H */

#include <lttng/tracepoint-event.h>
