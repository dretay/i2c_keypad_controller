#include "multipart_i2c.h"

//given a pb message containing a union, iterate over the pb field array and look for a matching 
//tag to encode the message with
bool encode_multipartmessage(pb_ostream_t *stream, const pb_field_t messagetype[], const void *message) {
	const pb_field_t *field;
	for (field = MultipartMessage_fields; field->tag != 0; field++) {
		if (field->ptr == messagetype) {
		    /* This is our field, encode the message using it. */
			if (!pb_encode_tag_for_field(stream, field)) {
				return false;
			}
            
			return pb_encode_submessage(stream, messagetype, message);
		}
	}
    
	/* Didn't find the field for messagetype */
	return false;
}

//given a pb stream with a union message, derrive which type the message is composed of
const pb_field_t* decode_unionmessage_type(pb_istream_t *stream) {
	pb_wire_type_t wire_type;
	uint32_t tag;
	bool eof;

	while (pb_decode_tag(stream, &wire_type, &tag, &eof)) {
		if (wire_type == PB_WT_STRING) {
			const pb_field_t *field;
			for (field = MultipartMessage_fields; field->tag != 0; field++) {
				if (field->tag == tag && (field->type & PB_LTYPE_SUBMESSAGE)) {
					/* Found our field. */
					return field->ptr;
				}
			}
		}

		/* Wasn't our field.. */
		pb_skip_field(stream, wire_type);
	}

	return NULL;
}

//given a union message and a derrived type, decode into the appropriate concrete type
bool decode_unionmessage_contents(pb_istream_t *stream, const pb_field_t fields[], void *dest_struct) {
	pb_istream_t substream;
	bool status;
	if (!pb_make_string_substream(stream, &substream)) {
		return false;
	}

	status = pb_decode(&substream, fields, dest_struct);
	pb_close_string_substream(stream, &substream);
	return status;
}

//given a pb message stream figure out how many messages are necessary to xmit the 
//data and send that info to the client
bool send_multipart_config(void* i2c_instance, uint8_t bytes_written, uint8_t *i2c_message_buffer, uint8_t i2c_address) {
	uint32_t err_code;
	MultipartMessageConfig multipart_message_config = MultipartMessageConfig_init_default;
	pb_ostream_t my_multipart_message_stream;

	multipart_message_config.word_count = (bytes_written + MAX_I2C_MSG - 1) / MAX_I2C_MSG;
	multipart_message_config.word_size = MAX_I2C_MSG;
	my_multipart_message_stream = pb_ostream_from_buffer(i2c_message_buffer, MAX_I2C_BUFFER);
	if (!encode_multipartmessage(&my_multipart_message_stream, MultipartMessageConfig_fields, &multipart_message_config)) {
		return false;		
	}
	return i2c_tx(i2c_instance, i2c_address, i2c_message_buffer, my_multipart_message_stream.bytes_written);
}

//send the actual multipart message by chunking the pb stream into smaller messages and sending them over the wire
bool send_multipart_message(void* i2c_instance, pb_ostream_t *multipart_message_stream, uint8_t *i2c_message_buffer, uint8_t i2c_address) {
	int i = 0;
	bool tx_success = true;
	uint8_t temp_config_message_buffer[MAX_I2C_MSG] = { 0 };
	//xfer the config over
	for (i = 0; i < multipart_message_stream->bytes_written; i++) {
		if ((i > 0) && (i % MAX_I2C_MSG == 0)) {
			tx_success &= i2c_tx(i2c_instance, i2c_address, temp_config_message_buffer, MAX_I2C_MSG);
			if (!tx_success) {
				break;
			}
		}
		temp_config_message_buffer[i % MAX_I2C_MSG] = i2c_message_buffer[i];
	}
	if (multipart_message_stream->bytes_written % MAX_I2C_MSG != 0 && tx_success) {
		tx_success &= i2c_tx(i2c_instance, i2c_address, temp_config_message_buffer, (multipart_message_stream->bytes_written % MAX_I2C_MSG));
	}
	return tx_success;
}

//sends a pb object over i2c to a specified address
bool send_i2c_msg(void* pb_msg, void* i2c_instance, const pb_field_t pb_msg_fields[], uint8_t i2c_address) {
	pb_ostream_t multipart_message_stream;
	uint8_t i2c_message_buffer[MAX_I2C_BUFFER] = { 0 };	

	//encode once to get the size of the keypad config object and use the stream to setup a multipart message
	multipart_message_stream = pb_ostream_from_buffer(i2c_message_buffer, MAX_I2C_BUFFER);
	if (!encode_multipartmessage(&multipart_message_stream, pb_msg_fields, pb_msg)) {
		return false;		
	}	

	//if the message is too big to send in a single go, break up into a multipart message
	if (multipart_message_stream.bytes_written > MAX_I2C_MSG) {
		//send a configuration message indicating the size of the message to be transmitted
		if (!send_multipart_config(i2c_instance, multipart_message_stream.bytes_written, i2c_message_buffer, i2c_address)) {
			return false;
		}
	
		//re-encode the keypad config - could save this step if we just used 2 buffers but w/e
		multipart_message_stream = pb_ostream_from_buffer(i2c_message_buffer, MAX_I2C_BUFFER);
		if (!encode_multipartmessage(&multipart_message_stream, KeypadConfig_fields, pb_msg)) {
			return false;		
		}
	}

	//send the actual message
	if (!send_multipart_message(i2c_instance, &multipart_message_stream, i2c_message_buffer, i2c_address)) {
		return false;
	}

	return true;
}
