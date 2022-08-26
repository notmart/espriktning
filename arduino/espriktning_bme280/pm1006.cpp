// GPLv2 2022 ERR e@richiardone.eu

#include "pm1006.h"


PM1006::PM1006(Stream * serial){
  _serial = serial;
  
  memset(_rxbuf, 0, sizeof(_rxbuf));
}


bool PM1006::read_pm25(uint16_t *pm){
  
  #ifdef VERBOSE
  Serial.printf("Reading PM2.5 value... ");
  #endif
    
  if(request()){
    *pm = (_rxbuf[5] << 8) + _rxbuf[6];

    #ifdef VERBOSE
    Serial.printf("\nRead: %d ug/m^3\n", *pm);
    #endif
    
    return true;
  }

  #ifdef VERBOSE
  Serial.printf("failed!\n");
  #endif
  
  return false;
}


// Sends query and get response with timeout
bool PM1006::request(){
  uint8_t i = 0;

  _serial->write(PM_QUERY, PM_QUERY_LEN);

  unsigned long start = millis();
  while((millis() - start) < DEFAULT_TIMEOUT) {
    while(_serial->available()) {
      _rxbuf[i++] = _serial->read();
      if(i == RX_BUF_LEN){
        
        #ifdef VERBOSE
        int i;
        for(i = 0; i < RX_BUF_LEN; i++){
          Serial.printf(" %02hhX", _rxbuf[i]);  
        }
        #endif
        
        return check_rx();
      }
    }
    yield();
  }

  return false;
}

// Simple parsing of response
bool PM1006::check_rx(){
  uint8_t sum = 0;
  uint8_t i;

  if(_rxbuf[0] == 0x16){
    if(_rxbuf[1] == 0x11){
      if(_rxbuf[2] == 0x0b){
        
        for(i = 0; i < (RX_BUF_LEN - 1); i++) {
          sum += _rxbuf[i];
        }

        if((256 - sum) == _rxbuf[i]){
          return true;
        }
      }
    }
  }
  
  return false;
}
