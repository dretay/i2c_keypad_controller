#pragma once


#include <pb.h>
#include <pb_common.h>
#include <pb_encode.h>
#include <pb_decode.h>

//TODO: can the multipart stuff be broken out into its own file? can you span or inherit from other files?
#include "multipart_messages.pb.h"

  
#define MAX_I2C_MSG 31
#define MAX_I2C_BUFFER 128


//internal stuff - should not be called directly
bool encode_multipartmessage(pb_ostream_t *stream, const pb_field_t messagetype[], const void *message);
const pb_field_t* decode_unionmessage_type(pb_istream_t *stream);
bool decode_unionmessage_contents(pb_istream_t *stream, const pb_field_t fields[], void *dest_struct);
bool send_multipart_config(void* i2c_instance, uint8_t bytes_written, uint8_t *i2c_message_buffer, uint8_t i2c_address);
bool send_multipart_message(void* i2c_instance, pb_ostream_t *multipart_message_stream, uint8_t *i2c_message_buffer, uint8_t i2c_address);

//the only method that really matters to consumers
bool send_i2c_msg(void* pb_msg, void* i2c_instance, const pb_field_t pb_msg_fields[], uint8_t i2c_address);

//this needs to be implemented as a platform-specific detail
extern bool i2c_tx(void* i2c_instance, uint8_t i2c_address, uint8_t *message, uint8_t size);
