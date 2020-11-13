#pragma once

#include "app_config.h"
#ifdef WITH_CAN

#include <stdint.h>

#include <canard.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Types
//

typedef struct {
	uint32_t uptime;
	uint8_t health;
	uint8_t mode;
	uint8_t vssc;
} uavcan_heartbeat_t;

typedef struct {
	uint8_t major;
	uint8_t minor;
} uavcan_version_t;

typedef struct {
	uavcan_version_t protocol_ver;
	uavcan_version_t hw_ver;
	uavcan_version_t sw_ver;
	uint64_t vcs_revision;
	uint8_t uuid[16];
	const char *name;
	// not used - can be disabled to save RAM
#ifdef UAVCAN_FULL_STRUCTS
	bool has_software_image_crc;
	uint64_t software_image_crc;
	uint8_t certificate_of_authenticity[222];
	uint8_t certificate_of_authenticity_len;
#endif
} uavcan_node_info_t;

//
// Callbacks
//

int uavcan_init2();

void uavcan_on_heartbeat(const CanardNodeID remote_node_id,
			 const uint32_t uptime, const uint8_t health,
			 const uint8_t mode, const uint8_t vssc);
uint8_t uavcan_on_set_dos_req(const CanardNodeID remote_node_id,
			      const uint8_t index, const uint8_t *const data,
			      const uint8_t length);
uint8_t uavcan_on_set_aos_req(const CanardNodeID remote_node_id,
			      const uint8_t index, const uint8_t *const data,
			      const uint8_t length);
uint8_t uavcan_on_get_dis_req(const CanardNodeID remote_node_id,
			      const uint8_t index, uint8_t *const data,
			      const uint8_t length);
uint8_t uavcan_on_get_ais_req(const CanardNodeID remote_node_id,
			      const uint8_t index, uint8_t *const data,
			      const uint8_t length);
void uavcan_on_set_dos_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index);
void uavcan_on_set_aos_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index);
void uavcan_on_get_dis_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index,
			    const uint8_t *const data, const uint8_t length);
void uavcan_on_get_ais_resp(const CanardNodeID remote_node_id,
			    const uint8_t result, const uint8_t index,
			    const uint8_t *const data, const uint8_t length);

//
// Functions
//

int uavcan_init();

void uavcan_send_packets();
void uavcan_print_frame(const char *direction, const CanardFrame *frame);

int uavcan_send_heartbeat();

int uavcan_send_set_dos(const uint8_t node_id, const uint8_t index,
			const bool *values, const uint8_t values_len);
int uavcan_send_set_aos(const uint8_t node_id, const uint8_t index,
			const uint16_t *values, const uint8_t values_len);
int uavcan_send_get_dis(const uint8_t node_id, const uint8_t index,
			const uint8_t length);
int uavcan_send_get_ais(const uint8_t node_id, const uint8_t index,
			const uint8_t length);

//
// Constants
//

#define UAVCAN_HEARTBEAT_PORT_ID 7509
#define UAVCAN_GET_INFO_PORT_ID 430

#define UAVCAN_SET_DOS_PORT_ID 200
#define UAVCAN_SET_AOS_PORT_ID 201
#define UAVCAN_GET_DIS_PORT_ID 210
#define UAVCAN_GET_AIS_PORT_ID 211

// Messages extents
#define UAVCAN_SET_DOS_REQ_EXT 21
#define UAVCAN_SET_DOS_RESP_EXT 7
#define UAVCAN_SET_AOS_REQ_EXT 21
#define UAVCAN_SET_AOS_RESP_EXT 7
#define UAVCAN_GET_DIS_REQ_EXT 7
#define UAVCAN_GET_DIS_RESP_EXT 21
#define UAVCAN_GET_AIS_REQ_EXT 7
#define UAVCAN_GET_AIS_RESP_EXT 21

#define UAVCAN_ANALOG_VALUES_MAX_LENGTH 4
#define UAVCAN_DIGITAL_VALUES_MAX_LENGTH 64

typedef enum {
	UAVCAN_NODE_MODE_1_0_OPERATIONAL = 0,
	UAVCAN_NODE_MODE_1_0_INITIALIZATION,
	UAVCAN_NODE_MODE_1_0_MAINTENANCE,
	UAVCAN_NODE_MODE_1_0_SOFTWARE_UPDATE,
} uavcan_node_mode_t;

typedef enum {
	UAVCAN_NODE_HEALTH_1_0_NOMINAL = 0,
	UAVCAN_NODE_HEALTH_1_0_ADVISORY,
	UAVCAN_NODE_HEALTH_1_0_CAUTION,
	UAVCAN_NODE_HEALTH_1_0_WARNING,
} uavcan_node_health_t;

typedef enum {
	UAVCAN_GET_SET_RESULT_OK = 0,
	UAVCAN_GET_SET_RESULT_BAD_ARGUMENT,
	UAVCAN_GET_SET_RESULT_HW_ERROR,
} uavcan_pealc_getset_result_t;

//
// Globals
//

extern CanardInstance uavcan_canard;
extern uavcan_node_info_t uavcan_node_info;
extern uavcan_heartbeat_t uavcan_node_status;

#ifdef __cplusplus
}
#endif

#endif // ifdef WITH_CAN