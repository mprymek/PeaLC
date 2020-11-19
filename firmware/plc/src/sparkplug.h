#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	const char *const metric;
} sp_io_block_t;

typedef struct {
	const char *name;
	uint8_t datatype;
	uint64_t alias;
	/*
    union {
        uint32_t int_value;
        uint64_t long_value;
        float float_value;
        double double_value;
        bool boolean_value;
        char* string_value;
        //pb_callback_t bytes_value;
        //org_eclipse_tahu_protobuf_Payload_DataSet dataset_value;
        //org_eclipse_tahu_protobuf_Payload_Template template_value;
        //org_eclipse_tahu_protobuf_Payload_Metric_MetricValueExtension extension_value;
    } value;
    */
	void *value;
} metric_simple_t;

// from sparkplug_b.proto
enum { SP_DT_UINT16 = 6,
       SP_DT_UINT32 = 7,
       SP_DT_BOOLEAN = 11,
};

int sp_init();
int sp_update_output_block(io_block_t *block);
void sp_send_nbirth();

// TODO: should not be needed
int sp_send_metric_simple(const metric_simple_t *metric_simple,
			  const char *topic);

#ifdef __cplusplus
}
#endif

#if 0
#include <stdint.h>

#include "mqtt_client.h"

// callbacks
void sp_handle_set_metric(uint64_t alias, uint32_t value);

int sp_mqtt_on_message(esp_mqtt_event_handle_t event);
void sp_send_dbirth();
int sp_send_metric_simple(const metric_simple_t *metric_simple,
			  const char *topic);
int sp_device_dead(const char *device);
int sp_device_alive(const char *device);

// copied from sparkplug_b.proto
enum { SP_DT_UINT32 = 7,
       SP_DT_BOOLEAN = 11,
};

#endif