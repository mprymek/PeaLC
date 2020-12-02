#include "app_config.h"
#include "sparkplug.h"
#include "hal.h"

static bool fake_bool = false;
static size_t node_metrics_num;
static metric_simple_t node_metrics[SP_MAX_METRICS] = {
	{ .name = "Node Control/Rebirth",
	  .datatype = SP_DT_BOOLEAN,
	  .value = &fake_bool },
	{ .name = "Node Control/Reboot",
	  .datatype = SP_DT_BOOLEAN,
	  .value = &fake_bool },
};

// initialize metrics list
int sp_init()
{
	size_t nmi = 2;
	uint8_t addr;

	// add digital inputs
	addr = 0;
	for (size_t i = 0; i < digital_inputs_blocks_len; i++) {
		io_block_t *block = &digital_inputs_blocks[i];

		if (!block->enabled ||
		    block->driver_type != IO_DRIVER_SPARKPLUG) {
			continue;
		}
		if (nmi >= SP_MAX_METRICS) {
			die(DEATH_INIT_FAILED);
		}
		sp_io_block_t *data = block->driver_data;
		node_metrics[nmi].name = data->metric;
		node_metrics[nmi].datatype = SP_DT_BOOLEAN;
		node_metrics[nmi].value = block->buff;
		nmi++;
	}

	// add analog inputs
	for (size_t i = 0; i < analog_inputs_blocks_len; i++) {
		io_block_t *block = &analog_inputs_blocks[i];

		if (!block->enabled ||
		    block->driver_type != IO_DRIVER_SPARKPLUG) {
			continue;
		}
		if (nmi >= SP_MAX_METRICS) {
			die(DEATH_INIT_FAILED);
		}
		sp_io_block_t *data = block->driver_data;
		node_metrics[nmi].name = data->metric;
		node_metrics[nmi].datatype = SP_DT_UINT16;
		node_metrics[nmi].value = block->buff;
		nmi++;
	}

	// add digital outputs
	for (size_t i = 0; i < digital_outputs_blocks_len; i++) {
		io_block_t *block = &digital_outputs_blocks[i];

		if (!block->enabled ||
		    block->driver_type != IO_DRIVER_SPARKPLUG) {
			continue;
		}
		if (nmi >= SP_MAX_METRICS) {
			die(DEATH_INIT_FAILED);
		}
		sp_io_block_t *data = block->driver_data;
		node_metrics[nmi].name = data->metric;
		node_metrics[nmi].datatype = SP_DT_BOOLEAN;
		node_metrics[nmi].value = block->buff;
		nmi++;
	}

	// add analog outputs
	for (size_t i = 0; i < analog_outputs_blocks_len; i++) {
		io_block_t *block = &analog_outputs_blocks[i];

		if (!block->enabled ||
		    block->driver_type != IO_DRIVER_SPARKPLUG) {
			continue;
		}
		if (nmi >= SP_MAX_METRICS) {
			die(DEATH_INIT_FAILED);
		}
		sp_io_block_t *data = block->driver_data;
		node_metrics[nmi].name = data->metric;
		node_metrics[nmi].datatype = SP_DT_UINT16;
		node_metrics[nmi].value = block->buff;
		nmi++;
	}

	node_metrics_num = nmi;
	PRINTF("%u metrics ", node_metrics_num);

	return 0;
}

int sp_update_output_block(const io_type_t io_type, io_block_t *const block)
{
	sp_io_block_t *data = block->driver_data;

	if (!block->dirty) {
		return IO_OK;
	}

	uint8_t result;

	switch (io_type) {
	case IO_TYPE_DIGITAL:
		for (size_t i = 0; i < block->length; i++) {
			bool *value = &((bool *)block->buff)[i];
			metric_simple_t metric = {
				.datatype = SP_DT_BOOLEAN,
				.name = data->metric,
				.value = value,
			};
			sp_send_metric_simple(&metric, SP_TOPIC_NODE("NDATA"));
		}
		block->dirty = false;
		break;
	case IO_TYPE_ANALOG:
		for (size_t i = 0; i < block->length; i++) {
			uint16_t *value = &((uint16_t *)block->buff)[i];
			metric_simple_t metric = {
				.datatype = SP_DT_UINT16,
				.name = data->metric,
				.value = value,
			};
			sp_send_metric_simple(&metric, SP_TOPIC_NODE("NDATA"));
		}
		block->dirty = false;
		break;
	}

	return IO_OK;
}

#include <mqtt_client.h>

#include "pb_decode.h"
#include "pb_encode.h"
#include "sparkplug_b.pb.h"
#include "sparkplug.h"

static uint8_t seq = 0;

int sp_send_metric_simple(const metric_simple_t *metric_simple,
			  const char *topic);

void sp_handle_set_metric(const char *metric_name, uint64_t alias,
			  uint32_t value)
{
	if (!strcmp(metric_name, "Node Control/Rebirth")) {
		sp_send_nbirth();
		return;
	}
	if (!strcmp(metric_name, "Node Control/Reboot")) {
		hal_restart();
		return;
	}

	log_debug("-> SP(%s) = %u", metric_name, value);
	for (size_t bi = 0; bi < digital_inputs_blocks_len; bi++) {
		io_block_t *block = &digital_inputs_blocks[bi];

		if (block->driver_type != IO_DRIVER_SPARKPLUG) {
			continue;
		}

		sp_io_block_t *data = block->driver_data;
		if (!strcmp(metric_name, data->metric)) {
			bool *buff_value = &((bool *)block->buff)[0];
			if (value != *buff_value) {
				*buff_value = value;
				block->dirty = true;
			}
			// send confirmation
			bool value2 = value;
			metric_simple_t metric = {
				.datatype = SP_DT_BOOLEAN,
				.name = metric_name,
				.value = &value2,
			};
			sp_send_metric_simple(&metric, SP_TOPIC_NODE("NDATA"));
			return;
		}
	}

	for (size_t bi = 0; bi < analog_inputs_blocks_len; bi++) {
		io_block_t *block = &analog_inputs_blocks[bi];

		if (block->driver_type != IO_DRIVER_SPARKPLUG) {
			continue;
		}

		sp_io_block_t *data = block->driver_data;
		if (!strcmp(metric_name, data->metric)) {
			uint16_t *buff_value = &((uint16_t *)block->buff)[0];
			if (value != *buff_value) {
				*buff_value = value;
				block->dirty = true;
			}
			// send confirmation
			uint16_t value2 = value;
			metric_simple_t metric = {
				.datatype = SP_DT_UINT16,
				.name = metric_name,
				.value = &value2,
			};
			sp_send_metric_simple(&metric, SP_TOPIC_NODE("NDATA"));
		}
		return;
	}
}

// ---------------------------------------------- WIP --------------------------

#define METRIC_NAME_MAX 128
#define MESSAGE_BUFF_LEN 512

typedef struct {
	uint64_t timestamp;
	uint8_t metrics_num;
	const metric_simple_t *metrics;
} encode_simple_metrics_arg_t;

static uint64_t start_time_ms = 0;

static int sp_send_xbirth(const metric_simple_t *metrics, uint8_t metrics_num,
			  const char *topic);
static uint64_t sp_now();
static bool
encode_payload_simple_metrics(pb_ostream_t *ostream,
			      const encode_simple_metrics_arg_t *const arg);

// callbacks
void sp_handle_set_metric(const char *metric, uint64_t alias, uint32_t value);

void sp_send_nbirth()
{
	// SparkPlug specification:
	// A NBIRTH message must always contain a sequence number of zero.
	seq = 0;
	sp_send_xbirth(node_metrics, node_metrics_num, SP_TOPIC_NODE("NBIRTH"));
}

static int sp_send_xbirth(const metric_simple_t *metrics, uint8_t metrics_num,
			  const char *topic)
{
	uint8_t buff[MESSAGE_BUFF_LEN];
	pb_ostream_t ostream = pb_ostream_from_buffer(buff, MESSAGE_BUFF_LEN);

	encode_simple_metrics_arg_t arg;
	arg.timestamp = sp_now();
	arg.metrics_num = metrics_num;
	// TODO: bdSeq
	arg.metrics = metrics;

	if (!encode_payload_simple_metrics(&ostream, &arg)) {
		PRINTF("ERROR: xBIRTH encoding failed. topic=%s\n", topic);
		return -1;
	}

	if (mqtt_publish(topic, (const char *)buff, ostream.bytes_written) ==
	    0) {
		// 0 means failure
		return -1;
	}
	return 0;
}

static uint64_t sp_now()
{
	if (start_time_ms == 0) {
		return 0;
	}
	return (hal_uptime_usec() / 1000) + start_time_ms;
}

static bool encode_string(pb_ostream_t *stream, const pb_field_iter_t *field,
			  void *const *arg)
{
	char *str = (char *)*arg;
	if (!pb_encode_tag_for_field(stream, field))
		return false;
	return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}

static bool encode_simple_metrics(pb_ostream_t *ostream,
				  const pb_field_t *field, void *const *arg0)
{
	const encode_simple_metrics_arg_t *arg =
		(const encode_simple_metrics_arg_t *)*arg0;

	for (int i = 0; i < arg->metrics_num; i++) {
		const metric_simple_t *smetric = &(arg->metrics[i]);

		org_eclipse_tahu_protobuf_Payload_Metric metric =
			org_eclipse_tahu_protobuf_Payload_Metric_init_default;
		// RBF: test if timestamp is sane (TODO: birth only after clock is synchronized)
		if (arg->timestamp == 0) {
			metric.has_timestamp = false;
		} else {
			metric.has_timestamp = true;
			metric.timestamp = arg->timestamp;
		}
		metric.has_datatype = true;
		metric.datatype = smetric->datatype;
		switch (metric.datatype) {
		case SP_DT_BOOLEAN:
			metric.which_value =
				org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag;
			metric.value.boolean_value = *(bool *)smetric->value;
			break;
		case SP_DT_UINT16:
			metric.which_value =
				org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag;
			// NOTE: official libraries (JS and NodeRED) use long_value for datatype==7, IDKW
			metric.value.int_value = *(uint16_t *)smetric->value;
			break;
		case SP_DT_UINT32:
			metric.which_value =
				org_eclipse_tahu_protobuf_Payload_Metric_long_value_tag;
			// NOTE: official libraries (JS and NodeRED) use long_value for datatype==7, IDKW
			metric.value.long_value = *(uint32_t *)smetric->value;
			break;
		default:
			PRINTF("ERROR: bad datatype\n");
			return false;
		}
		metric.name.funcs.encode = encode_string;
		metric.name.arg = (void *)smetric->name;
		//metric.has_alias = true;
		//metric.alias = smetric->alias;

		if (!pb_encode_tag_for_field(ostream, field)) {
			PRINTF("ERROR: metric tag encoding failed!\n");
			return false;
		}
		if (!pb_encode_submessage(
			    ostream,
			    org_eclipse_tahu_protobuf_Payload_Metric_fields,
			    &metric)) {
			PRINTF("ERROR: metric encoding failed!\n");
			return false;
		}
	}
	return true;
}

static bool
encode_payload_simple_metrics(pb_ostream_t *ostream,
			      const encode_simple_metrics_arg_t *const arg)
{
	org_eclipse_tahu_protobuf_Payload payload =
		org_eclipse_tahu_protobuf_Payload_init_default;

	// RBF: test if timestamp is sane (TODO: birth only after clock is synchronized)
	if (arg->timestamp == 0) {
		payload.has_timestamp = false;
	} else {
		payload.has_timestamp = true;
		payload.timestamp = arg->timestamp;
	}
	payload.metrics.funcs.encode = encode_simple_metrics;
	payload.metrics.arg = (void *)arg;
	payload.has_seq = true;
	payload.seq = seq++;

	return pb_encode(ostream, org_eclipse_tahu_protobuf_Payload_fields,
			 &payload);
}

int sp_send_metric_simple(const metric_simple_t *metric_simple,
			  const char *topic)
{
	uint8_t buff[64];
	pb_ostream_t ostream = pb_ostream_from_buffer(buff, 64);

	encode_simple_metrics_arg_t arg;
	arg.timestamp = sp_now();
	arg.metrics_num = 1;
	arg.metrics = metric_simple;

	if (!encode_payload_simple_metrics(&ostream, &arg)) {
		PRINTF("ERROR: metrics encoding\n");
		return -1;
	}

	if (mqtt_publish(topic, (const char *)buff, ostream.bytes_written) ==
	    0) {
		// 0 means failure
		return -1;
	}
	return 0;
}

// TODO: correct implem.
int sp_device_dead(const char *device)
{
	uint8_t buff[MESSAGE_BUFF_LEN];
	pb_ostream_t ostream = pb_ostream_from_buffer(buff, MESSAGE_BUFF_LEN);
	char *topic = SP_TOPIC_DEVICE("farm", "DDEATH");
	extern bool farm_alive;
	farm_alive = false;

	encode_simple_metrics_arg_t arg;
	arg.timestamp = sp_now();
	arg.metrics_num = 0;

	if (!encode_payload_simple_metrics(&ostream, &arg)) {
		PRINTF("ERROR: xDEATH encoding failed. topic=%s\n", topic);
		return -1;
	}

	if (mqtt_publish(topic, (const char *)buff, ostream.bytes_written) ==
	    0) {
		// 0 means failure
		return -1;
	}
	return 0;
}

static bool metric_name_decode(pb_istream_t *istream, const pb_field_t *field,
			       void **arg)
{
	if (istream->bytes_left > METRIC_NAME_MAX) {
		PRINTF("ERROR: metric name too long\n");
		return false;
	}
	return pb_read(istream, (uint8_t *)*arg, istream->bytes_left);
}

// TODO: This is probably completely wrong. Should be just a decoder, not callback!
// sp_handle_set_metric should be called from sp_mqtt_on_message.
static bool metric_decode(pb_istream_t *istream, const pb_field_t *field,
			  void **arg)
{
	char metric_name[METRIC_NAME_MAX + 1];
	org_eclipse_tahu_protobuf_Payload_Metric metric =
		org_eclipse_tahu_protobuf_Payload_Metric_init_default;
	// TODO string is not zero-ended!
	memset(metric_name, 0, METRIC_NAME_MAX + 1);
	metric.name.funcs.decode = metric_name_decode;
	metric.name.arg = &metric_name;
	if (!pb_decode(istream, org_eclipse_tahu_protobuf_Payload_Metric_fields,
		       &metric)) {
		PRINTF("ERROR: protobuf msg decoding\n");
		return false;
	}
	PRINTF("SP CMD: alias %llu = ", metric.alias);
	switch (metric.which_value) {
	case org_eclipse_tahu_protobuf_Payload_Metric_int_value_tag:
		PRINTF("%u\n", metric.value.int_value);
		sp_handle_set_metric(metric_name, metric.alias,
				     metric.value.int_value);
		break;
	case org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag:
		PRINTF("%u\n", metric.value.boolean_value);
		sp_handle_set_metric(metric_name, metric.alias,
				     metric.value.boolean_value);
		break;
	default:
		PRINTF("ERROR: unsupported value type %d\n",
		       metric.which_value);
	}
	return true;
}

// TODO: ESP32-specific type here!
int sp_mqtt_on_message(esp_mqtt_event_handle_t event)
{
	if (strncmp(event->topic, SP_TOPIC_NODE("NCMD"), event->topic_len) ==
	    0) {
		org_eclipse_tahu_protobuf_Payload payload =
			org_eclipse_tahu_protobuf_Payload_init_default;
		payload.metrics.funcs.decode = metric_decode;
		pb_istream_t istream = pb_istream_from_buffer(
			(pb_byte_t *)event->data, event->data_len);
		if (!pb_decode(&istream,
			       org_eclipse_tahu_protobuf_Payload_fields,
			       &payload)) {
			PRINTF("ERROR: protobuf msg decoding failed\n");
		}
		return true;
	}

	return false;
}

/*
// full-featured metrics encoding - not used at the moment

// arg = array of pointers to metric
typedef org_eclipse_tahu_protobuf_Payload_Metric **encode_metrics_arg_t;

static bool encode_metrics(pb_ostream_t *ostream, const pb_field_t *field, void *const *arg) {
    encode_metrics_arg_t metrics = (encode_metrics_arg_t) *arg;

    for(int i=0; metrics[i]!=NULL; i++) {
        org_eclipse_tahu_protobuf_Payload_Metric *metric = metrics[i];
        PRINTF("metric %d: name=%s alias=%llu datatype=%u\n", i, (char*)metric->name.arg, metric->alias, metric->datatype);
        if (!pb_encode_tag_for_field(ostream, field)) {
            PRINTF("ERROR: metric tag encoding failed!\n");
            return false;
        }
        if(!pb_encode_submessage(ostream, org_eclipse_tahu_protobuf_Payload_Metric_fields, metric)) {
            PRINTF("ERROR: metric encoding failed!\n");
            return false;
        }
    }
    return true;
}

    org_eclipse_tahu_protobuf_Payload_Metric metrics[MAX_METRICS];

    uint64_t now = sp_now();

    //metric.datatype = SP_DT_UINT32;
    //metric.value.int_value = hal_uptime_msec();

    org_eclipse_tahu_protobuf_Payload_Metric rebirth_metric = org_eclipse_tahu_protobuf_Payload_Metric_init_default;
    rebirth_metric.has_timestamp = true;
    rebirth_metric.timestamp = now;
    rebirth_metric.has_datatype = true;
    rebirth_metric.datatype = SP_DT_BOOLEAN;
    rebirth_metric.which_value = org_eclipse_tahu_protobuf_Payload_Metric_boolean_value_tag;
    rebirth_metric.value.boolean_value = false;
    rebirth_metric.name.funcs.encode = encode_string;
    rebirth_metric.name.arg = &"Node Control/Rebirth";
    rebirth_metric.has_alias = true;
    rebirth_metric.alias = 1;

    org_eclipse_tahu_protobuf_Payload_Metric *metrics[] = {
        &rebirth_metric, NULL
    };

    org_eclipse_tahu_protobuf_Payload payload = org_eclipse_tahu_protobuf_Payload_init_default;
    payload.has_timestamp = true;
    payload.timestamp = now;
    payload.metrics.funcs.encode = encode_metrics;
    payload.metrics.arg = &metrics;
    payload.has_seq = true;
    payload.seq = seq++;
*/
