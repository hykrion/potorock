#include "bsky.h"

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <time.h>

#define POTO_DEBUG 1

const char* fingerprint PROGMEM = "4A:CA:75:BD:A3:FF:27:FD:91:57:18:82:B3:DE:F7:0F:90:EF:B9:08";  // BSKY

/**
*/
Bsky::Bsky(String id, String psw)
{
  m_id = id;
  m_psw = psw;
  m_nextPostHourIndex = 0;
}

/**
*/
void Bsky::post(String text)
{
  String dateTime(get_date_time());
  HTTPClient https;
  WiFiClientSecure wifiClientSecure;
  
  read_token();
  String bearer = "Bearer "+ m_authToken;
  
  https.begin(wifiClientSecure, "bsky.social", 443, "/xrpc/com.atproto.repo.createRecord", true);
  wifiClientSecure.setFingerprint(fingerprint);

  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", bearer);
  String payload = R"(
    {
      "collection": "app.bsky.feed.post",
      "repo": ")"+ m_id +R"(",
      "record": {
        "text": ")"+ text +R"(",
        "createdAt": ")"+ dateTime +R"("
      }
    }
  )";
  int httpResponseCode = https.POST(payload);

  if (httpResponseCode > 0)
  {
    String response = https.getString();
    Serial.print("Mensaje enviado con éxito:");
    Serial.println(response);
  }
  else if (POTO_DEBUG)
  {
    Serial.print("Error al enviar mensaje: ");
    Serial.println(httpResponseCode);
  }

  https.end();
}

/**
 * 
*/
boolean
Bsky::next_post()
{
  int postHours[] = {8, 14, 21};
  boolean result = false;
  WiFiUDP ntpUDP;
  struct tm timeInfo;
  long GMT1 = 3600;                   // 1h
  NTPClient timeClient(ntpUDP, "0.es.pool.ntp.org", GMT1);
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  timeInfo = *localtime(&rawTime);

  if(timeClient.isTimeSet())
  {
    if (postHours[m_nextPostHourIndex] == timeClient.getHours())
    {
      m_nextPostHourIndex = (m_nextPostHourIndex + 1) % (sizeof(postHours) / sizeof(*postHours));
      result = true;
    }
  }

  return result;
}

// ------------------------------------------------------------------
// PRIVATE
// ------------------------------------------------------------------
/**
*/
void
Bsky::read_token()
{
  HTTPClient https;
  WiFiClientSecure wifiClientSecure;
  
  https.begin(wifiClientSecure, "bsky.social", 443, "/xrpc/com.atproto.server.createSession", true);
  wifiClientSecure.setFingerprint(fingerprint);

  https.addHeader("Content-Type", "application/json");
  String payload = R"(
    {
      "identifier": ")" + m_id +R"(",
      "password": ")" + m_psw + R"("
    }
  )";
  int httpResponseCode = https.POST(payload);

  if (httpResponseCode > 0)
  {
    String response = https.getString();
    
    if (POTO_DEBUG)
    {
      Serial.println("Mensaje enviado con éxito:");
      Serial.println(response);
    }
    DynamicJsonDocument json(1024);
    deserializeJson(json, response);
    m_authToken = json["accessJwt"].as<String>();
    
    if (POTO_DEBUG)
    {
      Serial.print("token: ");
      Serial.println(m_authToken);
    }
  }
  else if (POTO_DEBUG)
  {
    Serial.print("Error al enviar mensaje: ");
    Serial.println(httpResponseCode);
    Serial.println(payload);
  }

  https.end();
}

/**
*/
String
Bsky::get_date_time()
{
  char buffer[80];
  WiFiUDP ntpUDP;
  long GMT1 = 3600;                   // 1h
  NTPClient timeClient(ntpUDP, "0.es.pool.ntp.org", GMT1);
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime();
  struct tm timeInfo;

  timeInfo = *localtime(&rawTime);
  strftime(buffer, sizeof(buffer), "%FT%TZ", &timeInfo);

  return String(buffer);
}