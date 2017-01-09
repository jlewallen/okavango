#include <Arduino.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "fkcomms.pb.h"

bool encode_fields(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
    fkcomms_Field messageField = {};

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    if (!pb_encode_submessage(stream, fkcomms_Field_fields, &messageField))
        return false;

    return true;
}

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        delay(100);
    }

    uint8_t buffer[128];
    fkcomms_Message message = {};
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    message.id = 1;
    message.time = 1483984821L;
    message.fields.arg = buffer;
    message.fields.funcs.encode = encode_fields;

    bool status = pb_encode(&stream, fkcomms_Message_fields, &message);

    if (!status)
    {
        Serial.print("Encoding failed: ");
        Serial.println(PB_GET_ERROR(&stream));
    }

    Serial.println(stream.bytes_written);
}

void loop() {

}
