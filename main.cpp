#include <Arduino.h>

#define POTO_DEBUG 1

#include "poto.h"
#include "ts.h"
#include "bsky.h"

// Poto
const char* ssid = "YOUR_SSID";
const char* wifiPsw = "YOUR_WIFI_PSW";
const char* chatgpt_sk = "YOUR_SK_KEY";

// ThingSpeak
unsigned long TSChannel = YOUR_TS_CHANNEL;
const char* TSWriteAPIKey = "YOUR_TS_KEY";

// Bluesky
const char* bskyId = "YOUR_BS_ID";
const char* bskyPsw = "YOUR_BS_PSW";

enum {POTO_STA_WEATHER, POTO_STA_CHATGPT, POTO_STA_TS, POTO_STA_BSKY, POTO_STA_15_MIN, POTO_STA_WEATHER_15, POTO_STA_TS_15, POTO_STA_NEXT_POST};

Poto poto(ssid, wifiPsw, chatgpt_sk);
TS ts(TSChannel, TSWriteAPIKey);
Bsky bsky(bskyId, bskyPsw);

int state = POTO_STA_WEATHER;

void setup()
{
  poto.init();
}

void loop()
{
  switch (state)
  {
    case POTO_STA_WEATHER:
      poto.read_weather_data();
      state = POTO_STA_CHATGPT;
      break;
    case POTO_STA_CHATGPT:
      poto.read_chatgpt_message();
      state = POTO_STA_TS;
      break;
    case POTO_STA_TS:
      ts.send_data();
      state = POTO_STA_BSKY;
      break;
    case POTO_STA_BSKY:
      bsky.post(poto.get_post());
      state = POTO_STA_15_MIN;
      break;
    case POTO_STA_15_MIN:
      if (poto.min_15())
        state = POTO_STA_WEATHER_15;
      break;
    case POTO_STA_WEATHER_15:
      poto.read_weather_data();
      state = POTO_STA_TS_15;
      break;
    case POTO_STA_TS_15:
      ts.send_data();
      state = POTO_STA_NEXT_POST;
      break;
    case POTO_STA_NEXT_POST:
      if (bsky.next_post())
        state = POTO_STA_WEATHER;
      else
        state = POTO_STA_15_MIN;
      break;
  }
  delay(1000);
}
