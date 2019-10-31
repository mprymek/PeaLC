#ifdef __cplusplus
extern "C" {
#endif

int plc_init(void);

// ---------------------------------------------- MatIEC-compiled program API --

void config_init__(void);
void config_run__(unsigned long tick);
// "imported"
extern unsigned long long common_ticktime__;

// ---------------------------------------------- remote vars ------------------

typedef struct {
	uint8_t node_id;
	uint8_t index;
	uint8_t len;
	union {
		uint16_t *analog_vals;
		bool *digital_vals;
	};
} uavcan_vals_block_t;

typedef enum {
	PLC_STATE_RUNNING,
	PLC_STATE_PAUSED,
} plc_state_t;

void plc_set_state(plc_state_t state);

extern uavcan_vals_block_t uavcan_dis_blocks[];
extern uavcan_vals_block_t uavcan_dos_blocks[];
extern uavcan_vals_block_t uavcan_ais_blocks[];
extern uavcan_vals_block_t uavcan_aos_blocks[];

extern const uint8_t uavcan_dis_blocks_len;
extern const uint8_t uavcan_dos_blocks_len;
extern const uint8_t uavcan_ais_blocks_len;
extern const uint8_t uavcan_aos_blocks_len;

#define EXT_BUFF_SIZE (IO_BUFFER_SIZE - REMOTE_VARS_INDEX)
extern bool ext_dos[EXT_BUFF_SIZE];
extern uint16_t ext_aos[EXT_BUFF_SIZE];
extern bool ext_dis[EXT_BUFF_SIZE];
extern uint16_t ext_ais[EXT_BUFF_SIZE];

#ifdef __cplusplus
}
#endif
