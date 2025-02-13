#include "ts.h"

#include <WiFiClient.h>

#include "poto.h"
#include "ThingSpeak.h"

extern Poto poto;

#define POTO_DEBUG 1

// TS fields
#define TS_TEM_FIELD 1
#define TS_HUM_FIELD 2

TS::TS(unsigned long channel, const char* writeAPIKey)
{
  m_channel = channel;
  m_writeAPIKey = writeAPIKey;
}

/**
  NOTE  No soporta SSL
*/
void
TS::send_data()
{
  WiFiClient wifiClient;
  ThingSpeakClass ts;
  
  poto.read_weather_data();
  float tem = poto.get_temperature();
  float hum = poto.get_humidity();
 
  ts.begin(wifiClient); 
  ts.setField(TS_TEM_FIELD, tem);
  ts.setField(TS_HUM_FIELD, hum);
  int res = ts.writeFields(m_channel, m_writeAPIKey);
  
  if (POTO_DEBUG)
  {
    if(res == 200)
      Serial.println("Channel update successful.");
    else
      Serial.println("Problem updating channel. HTTP error code " + String(res));
  }
}

// ------------------------------------------------------------------
// PRIVATE
// ------------------------------------------------------------------
