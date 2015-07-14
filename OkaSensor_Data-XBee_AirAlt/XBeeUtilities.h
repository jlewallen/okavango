XBee xbee = XBee();

#pragma pack(push, 1)

typedef struct {
  float v1;
  float v2;
  float v3;
  float v4;
  unsigned long time;
  char kind;
} packet_t;

packet_t payload;

#pragma pack(push, 4)

XBeeAddress64 addr64 = XBeeAddress64(XBEE_DESTINATION_HIGH, XBEE_DESTINATION_LOW);
ZBTxRequest zbTx = ZBTxRequest(addr64, (uint8_t *)&payload, sizeof(payload));
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

AtCommandRequest request = AtCommandRequest();
AtCommandResponse response = AtCommandResponse();

bool sendAtCommand() {
  bool success = false;
  
  xbee.send(request);
  
  if (xbee.readPacket(1000)) {
    if (xbee.getResponse().getApiId() == AT_COMMAND_RESPONSE) {
      xbee.getResponse().getAtCommandResponse(response);
      
      if (response.isOk()) {
        Serial.print("C [");
        Serial.print(response.getCommand()[0]);
        Serial.print(response.getCommand()[1]);
        Serial.println("] OK");
        
        success = true;
      } 
      else {
        Serial.print("Error ");
        Serial.println(response.getStatus(), HEX);
      }
    } else {
      Serial.print("Error ");
      Serial.println(xbee.getResponse().getApiId(), HEX);
    }   
  } else {
    if (xbee.getResponse().isError()) {
      Serial.print("Error ");  
      Serial.println(xbee.getResponse().getErrorCode(), DEC);
    }
    else {
      Serial.println("No radio");
      delay(100);  
    }
  }
  return success;
}

uint8_t SM_CMD[] = {'S','M'};
uint8_t SN_CMD[] = {'S','N'};
uint8_t SP_CMD[] = {'S','P'};

void configureSleepMode() {
  Serial.println("SM");
  uint8_t byteParameter = 0x4;
  request.setCommandValue((uint8_t *)&byteParameter);
  request.setCommandValueLength(sizeof(byteParameter));
  request.setCommand(SM_CMD);
  while (!sendAtCommand()) {
  }

  Serial.println("SN");
  uint16_t wordParameter = 0xF;
  request.setCommandValue((uint8_t *)&wordParameter);
  request.setCommandValueLength(sizeof(wordParameter));
  request.setCommand(SN_CMD);
  while (!sendAtCommand()) {
  }

  Serial.println("SP");
  wordParameter = 0x7D0;
  request.setCommandValue((uint8_t *)&wordParameter);
  request.setCommandValueLength(sizeof(wordParameter) - 1);
  request.setCommand(SP_CMD);
  while (!sendAtCommand()) {
  }
}

void longDelayAndAttemptToSendPacket(uint32_t totalDelay) {
  uint32_t delayedSoFar = 0;
  uint8_t sentSuccessful = 0;
  while (delayedSoFar < totalDelay) {
      uint32_t thisPassStarted = millis();

      if (sentSuccessful) {
        delay(1000);
        Serial.print(delayedSoFar);
        Serial.print(" ");
        Serial.print(totalDelay);
        Serial.println("");
      }
      else {
        xbee.send(zbTx);
        if (xbee.readPacket(1000)) {
          if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
             xbee.getResponse().getZBTxStatusResponse(txStatus);
             if (txStatus.getDeliveryStatus() == SUCCESS) {
                Serial.print("Ok ");
                Serial.println(delayedSoFar / 1000);
                sentSuccessful = true;
             } else {
                Serial.println("F");
                delay(100);
             }
          }
        } else {
          Serial.println("No reply");
        }
      }

      delayedSoFar += millis() - thisPassStarted;
  }
}
