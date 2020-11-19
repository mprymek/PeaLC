#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"

int slave_init();
int update_input_block(io_block_t *block);
int update_output_block(io_block_t *block);

#ifdef __cplusplus
}
#endif
