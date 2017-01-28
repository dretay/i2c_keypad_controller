/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.7 at Sat Jan 28 02:28:52 2017. */

#ifndef PB_MULTIPART_MESSAGES_PB_H_INCLUDED
#define PB_MULTIPART_MESSAGES_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _KeypadConfig {
    pb_callback_t key_configuration;
    uint32_t irq_pin;
    uint32_t ring_buff_cnt;
    uint32_t reporting_delay;
/* @@protoc_insertion_point(struct:KeypadConfig) */
} KeypadConfig;

typedef struct _KeypadConfig_KeyConfig {
    uint32_t pin_no;
    uint32_t idx;
    bool has_pullup;
    bool pullup;
    bool has_invert;
    bool invert;
    bool has_debounce;
    uint32_t debounce;
/* @@protoc_insertion_point(struct:KeypadConfig_KeyConfig) */
} KeypadConfig_KeyConfig;

typedef struct _KeypadState {
    uint32_t buttons;
/* @@protoc_insertion_point(struct:KeypadState) */
} KeypadState;

typedef struct _MultipartMessageConfig {
    int32_t word_size;
    int32_t word_count;
/* @@protoc_insertion_point(struct:MultipartMessageConfig) */
} MultipartMessageConfig;

typedef struct _MultipartMessage {
    bool has_keypad_config;
    KeypadConfig keypad_config;
    bool has_multipart_message_config;
    MultipartMessageConfig multipart_message_config;
    bool has_keypad_state;
    KeypadState keypad_state;
/* @@protoc_insertion_point(struct:MultipartMessage) */
} MultipartMessage;

/* Default values for struct fields */
extern const uint32_t KeypadConfig_ring_buff_cnt_default;
extern const uint32_t KeypadConfig_reporting_delay_default;
extern const bool KeypadConfig_KeyConfig_pullup_default;
extern const bool KeypadConfig_KeyConfig_invert_default;
extern const uint32_t KeypadConfig_KeyConfig_debounce_default;

/* Initializer values for message structs */
#define KeypadState_init_default                 {0}
#define KeypadConfig_init_default                {{{NULL}, NULL}, 0, 5u, 50000u}
#define KeypadConfig_KeyConfig_init_default      {0, 0, false, true, false, true, false, 20u}
#define MultipartMessageConfig_init_default      {0, 0}
#define MultipartMessage_init_default            {false, KeypadConfig_init_default, false, MultipartMessageConfig_init_default, false, KeypadState_init_default}
#define KeypadState_init_zero                    {0}
#define KeypadConfig_init_zero                   {{{NULL}, NULL}, 0, 0, 0}
#define KeypadConfig_KeyConfig_init_zero         {0, 0, false, 0, false, 0, false, 0}
#define MultipartMessageConfig_init_zero         {0, 0}
#define MultipartMessage_init_zero               {false, KeypadConfig_init_zero, false, MultipartMessageConfig_init_zero, false, KeypadState_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define KeypadConfig_key_configuration_tag       1
#define KeypadConfig_irq_pin_tag                 2
#define KeypadConfig_ring_buff_cnt_tag           3
#define KeypadConfig_reporting_delay_tag         4
#define KeypadConfig_KeyConfig_pin_no_tag        1
#define KeypadConfig_KeyConfig_idx_tag           2
#define KeypadConfig_KeyConfig_pullup_tag        3
#define KeypadConfig_KeyConfig_invert_tag        4
#define KeypadConfig_KeyConfig_debounce_tag      5
#define KeypadState_buttons_tag                  1
#define MultipartMessageConfig_word_size_tag     1
#define MultipartMessageConfig_word_count_tag    2
#define MultipartMessage_keypad_config_tag       1
#define MultipartMessage_multipart_message_config_tag 2
#define MultipartMessage_keypad_state_tag        3

/* Struct field encoding specification for nanopb */
extern const pb_field_t KeypadState_fields[2];
extern const pb_field_t KeypadConfig_fields[5];
extern const pb_field_t KeypadConfig_KeyConfig_fields[6];
extern const pb_field_t MultipartMessageConfig_fields[3];
extern const pb_field_t MultipartMessage_fields[4];

/* Maximum encoded size of messages (where known) */
#define KeypadState_size                         6
/* KeypadConfig_size depends on runtime parameters */
#define KeypadConfig_KeyConfig_size              22
#define MultipartMessageConfig_size              22
#define MultipartMessage_size                    (38 + KeypadConfig_size)

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define MULTIPART_MESSAGES_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
