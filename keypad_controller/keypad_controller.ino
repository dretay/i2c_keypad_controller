#include <RingBuf.h>
#include <Wire.h>
#include <Button.h>
#include <TimerOne.h>
#include <PrintEx.h>

#include <pb.h>
#include <pb_decode.h>
#include <pb_common.h>
#include "keypad_messages.pb.h"


//adding printf support
StreamEx serial = Serial;

//todo: is there a more dynamic way to do this?
#define MAX_KEYPAD_BUTTONS 12
Button **keypad_buttons = new Button*[MAX_KEYPAD_BUTTONS];

uint8_t KEYPAD_BUTTONS_NUMBER;
enum tx_states {READY, WAITING, COMPLETE} tx_state = READY;
uint8_t KEYPAD_STATE_IRQ;

//todo: should i pick something better?
const byte SlaveDeviceId = 1;

RingBuf *my_buf;

typedef struct {
	uint8_t *buffer;
	uint8_t buffer_offset;
	uint8_t current_word;
	uint8_t word_count;
	bool in_progress;
} multipart_i2c_t;
multipart_i2c_t multipart_i2c;

uint16_t CURRENT_KEYPAD_STATE = 0;

#define MAX_I2C_MSG_SIZE 32

bool KEYPAD_CONFIGURED = false;

//grabbed verbatim from nanopb union example
//todo: should this be moved out into a lib?
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

//grabbed verbatim from nanopb union example
bool decode_unionmessage_contents(pb_istream_t *stream, const pb_field_t fields[], void *dest_struct) {
	pb_istream_t substream;
	bool status;
	if (!pb_make_string_substream(stream, &substream))
		return false;

	status = pb_decode(&substream, fields, dest_struct);
	pb_close_string_substream(stream, &substream);
	return status;
}

bool pin_listing_callback(pb_istream_t *stream, const pb_field_t *field, void **arg){
    
	KeypadConfig_KeyConfig key_configuration = KeypadConfig_KeyConfig_init_default;
    if (!pb_decode(stream, KeypadConfig_KeyConfig_fields, &key_configuration))
        return false;
	
	pinMode(key_configuration.pin_no, OUTPUT);

	serial.printf("(PIN) %d pullup: %d invert: %d debounce %d\n", key_configuration.pin_no, key_configuration.pullup, key_configuration.invert, key_configuration.debounce);
	
	keypad_buttons[KEYPAD_BUTTONS_NUMBER] = new Button(key_configuration.pin_no, key_configuration.pullup, key_configuration.invert, key_configuration.debounce);
	KEYPAD_BUTTONS_NUMBER++;
        
    return true;
}
void finish_config(KeypadConfig *keypad_config) {
	int i = 0;
	Timer1.initialize(keypad_config->reporting_delay);
	Timer1.attachInterrupt(service_irq);

	my_buf = RingBuf_new(sizeof(uint16_t), keypad_config->ring_buff_cnt);
	if (!my_buf) {
		serial.printf("Not enough memory to allocate ring buffer");
	}
	
	KEYPAD_STATE_IRQ = keypad_config->irq_pin;
	pinMode(KEYPAD_STATE_IRQ, OUTPUT);
	digitalWrite(KEYPAD_STATE_IRQ, false);
	
	//todo: shrink the button array if fewer than max are defined?
	KEYPAD_BUTTONS_NUMBER;	

	delay(2000);
	KEYPAD_CONFIGURED = true;
}

void receiveCallback(int in_byte_cnt) {	
	uint8_t i2c_buffer[MAX_I2C_MSG_SIZE] = { 0 };
	int i2c_buffer_idx = 0;
	bool status = false;
	pb_istream_t stream;
	const pb_field_t *type;

	while (Wire.available()) { i2c_buffer[i2c_buffer_idx++] = Wire.read(); }
	
	if (!multipart_i2c.in_progress) {
		stream = pb_istream_from_buffer(i2c_buffer, in_byte_cnt);
		type = decode_unionmessage_type(&stream);
		if (type == MultipartMessageConfig_fields) {
			MultipartMessageConfig msg = MultipartMessageConfig_init_default;
			status = decode_unionmessage_contents(&stream, MultipartMessageConfig_fields, &msg);			
			multipart_i2c.buffer = (uint8_t *)malloc(msg.word_count * msg.word_size);
			multipart_i2c.buffer_offset = 0;
			multipart_i2c.current_word = 0;
			multipart_i2c.word_count = msg.word_count;
			multipart_i2c.in_progress = true;
		}
	}
	//this will need to be revisited if I start transfering multiple types here...
	else {
		memcpy(&multipart_i2c.buffer[multipart_i2c.buffer_offset], &i2c_buffer, in_byte_cnt);
		multipart_i2c.buffer_offset += in_byte_cnt;		
		if (++multipart_i2c.current_word == multipart_i2c.word_count) {
			KeypadConfig keypad_configuration = KeypadConfig_init_default;
			keypad_configuration.key_configuration.funcs.decode = &pin_listing_callback;			

			stream = pb_istream_from_buffer(multipart_i2c.buffer, multipart_i2c.buffer_offset);			
			decode_unionmessage_type(&stream);
			status = decode_unionmessage_contents(&stream, KeypadConfig_fields, &keypad_configuration);
			if (status) {
				serial.printf("(KEYPAD) IRQ pin: %d Buffer: %d Delay %d", keypad_configuration.irq_pin, keypad_configuration.ring_buff_cnt, keypad_configuration.reporting_delay);
				finish_config(&keypad_configuration);				
			}
			else {
				serial.printf("!!!!!!!! unable to process multipart message");
			}
			free(multipart_i2c.buffer);
			multipart_i2c.buffer = NULL;
			multipart_i2c.in_progress = false;
		}
	}
}
void requestCallback() {
	uint8_t buffer[2];
	uint16_t state;
	
	Timer1.stop();
	if (!my_buf->isEmpty(my_buf)) {		
		my_buf->pull(my_buf, &state);
		buffer[0] = state >> 8;
		buffer[1] = state & 0xff;
		Wire.write(buffer, 2);    
	}
	else {
		Wire.write(buffer, 2);
	}
  tx_state = COMPLETE;
  Timer1.restart();
}
void service_irq(void) {	
  switch(tx_state){
    case READY:
      if (!my_buf->isEmpty(my_buf)) {                
        digitalWrite(KEYPAD_STATE_IRQ, true);
        tx_state = WAITING;
      }
      break;
    case WAITING: 
      break;
    case COMPLETE:
      digitalWrite(KEYPAD_STATE_IRQ, false);
      tx_state = READY;
      break;
  }  
}
void setup() {
	Serial.begin(115200);

	digitalWrite(SDA, 1);
	digitalWrite(SCL, 1);
	Wire.begin(SlaveDeviceId);
	Wire.onReceive(receiveCallback);	
	Wire.onRequest(requestCallback);
	
	//todo: maybe store the old config in eeprom?
	serial.printf("I2C setup complete\n");
	
}
static void print_state(uint16_t state) {
	uint8_t i;
	for (i = 0; i < KEYPAD_BUTTONS_NUMBER; i++) {
		if (bitRead(state, i)) {
			if (_DEBUG) Serial.print("1");
		}
		else {
			if (_DEBUG) Serial.print("0");
		}
	}
	if (_DEBUG) Serial.println("");
}

void loop() {
	uint8_t i;
	Button* curr_btn;
	bool state_changed = false;
	uint16_t keypad_state_cpy;

	if (KEYPAD_CONFIGURED) {
		for (i = 0; i < KEYPAD_BUTTONS_NUMBER; i++) {
			curr_btn = keypad_buttons[i];
			curr_btn->read();
			if (curr_btn->wasPressed()) {				
				state_changed = true;
				bitWrite(CURRENT_KEYPAD_STATE, i, 1);
			}
			else if (curr_btn->wasReleased()) {
				state_changed = true;
				bitWrite(CURRENT_KEYPAD_STATE, i, 0);
			}
		}
		if (state_changed) {
			if (_DEBUG) print_state(CURRENT_KEYPAD_STATE);
			memcpy(&keypad_state_cpy, &CURRENT_KEYPAD_STATE, sizeof(CURRENT_KEYPAD_STATE));
			my_buf->add(my_buf, &keypad_state_cpy);
		}
	}
}
