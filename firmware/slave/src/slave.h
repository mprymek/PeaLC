#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"

int slave_init();
int update_input_block(const io_type_t io_type, io_block_t *block);
int update_output_block(const io_type_t io_type, io_block_t *block);

#ifdef __cplusplus
}
#endif
