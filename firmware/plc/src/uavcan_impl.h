#include "app_config.h"
#ifdef WITH_CAN

#include "canard.h"

#ifdef __cplusplus
extern "C" {
#endif

int uavcan2_init(void);
void print_frame(const char *direction, const CanardCANFrame *frame);

#ifdef __cplusplus
}
#endif

#endif
