#include <iec_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

int plc_init(void);

//// MatIEC-compiled program API
void config_init__(void);
void config_run__(unsigned long tick);
// "imported"
extern unsigned long long common_ticktime__;
// "exported"
IEC_TIME __CURRENT_TIME;

#ifdef __cplusplus
}
#endif