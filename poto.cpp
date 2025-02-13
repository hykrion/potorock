#include "poto.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#include "Adafruit_Sensor.h"
#include "DHT.h"
#include "DHT_U.h"


#define POTO_DEBUG 1

#define LDR_PIN D1
#define DHTPIN  D4
#define DHTTYPE DHT11

const char* fingerprintChatGPT PROGMEM = "18:9D:B6:18:44:A1:6B:83:80:8F:14:CA:99:8B:51:CB:1F:0E:40:33";

/**
*/
Poto::Poto(String ssid, String wifiPsw, String chatgtp_sk)
{
  m_personalityIndex = 0;
  m_tem = 0.0;
  m_hum = 0.0;
  m_ssid = ssid;
  m_wifiPsw = wifiPsw;
  m_chatgtp_sk = chatgtp_sk;
  m_clock = millis();
}

/**
*/
void
Poto::init()
{
  init_pins();
  init_wifi();
}

/**
*/
void
Poto::read_weather_data()
{
  DHT_Unified dht(DHTPIN, DHTTYPE);
  dht.begin();
  sensors_event_t event;
  
  dht.temperature().getEvent(&event);
  m_tem = event.temperature;
  dht.humidity().getEvent(&event);
  m_hum = event.relative_humidity;

  if (POTO_DEBUG)
  {
    Serial.print("Temperatura: ");
    Serial.println(m_tem);
    Serial.print("Humedad: ");
    Serial.println(m_hum);
  }
}

/**
*/
float
Poto::get_temperature()
{
  return m_tem;
}

/**
*/
float
Poto::get_humidity()
{
  return m_hum;
}

/**
*/
void
Poto::read_chatgpt_message()
{
  HTTPClient https;
  WiFiClientSecure wifiClientSecure;
  String prompt;
  String completions;
  
  // Personalidad.
  String personality[] = {
    "un pirata",
    "la reina d'anglaterra",
    "blancaneus",
    "un presidiari",
    "Miguel de Cervantes",
    "Chiquito de la calzada"
  };

  if (POTO_DEBUG)
  {
    Serial.print("Personalidad: ");
    Serial.println(personality[m_personalityIndex]);
  }
  
  prompt += "Fa "+ String(m_tem) +" graus celsius i "+ String(m_hum) +" percent d'humitat.";
  prompt += "Dona aquesta informació con si fossis "+ personality[m_personalityIndex] +" en menys de 130 paruales.";
  m_personalityIndex = (m_personalityIndex + 1) % (sizeof(personality) / sizeof(*personality));
  
  https.begin(wifiClientSecure, "api.openai.com", 443, "/v1/chat/completions", true);
  wifiClientSecure.setFingerprint(fingerprintChatGPT);

  if (https.begin(wifiClientSecure, "api.openai.com", 443, "/v1/chat/completions", true))
  {
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", "Bearer "+ m_chatgtp_sk);
    String requestBody = R"(
      {
        "model": "gpt-3.5-turbo",
        "messages": [
          {"role": "system", "content": "You are a helpful assistant that speaks Catalan."},
          {"role": "user", "content": ")"+ prompt + R"("}
        ],
        "temperature": 0.7
      }
    )";
    int httpCode = https.POST(requestBody);
    
    if (POTO_DEBUG)
    {
      Serial.print("requestBody: ");
      Serial.println(requestBody);
    }
    
    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String response = https.getString();
        Serial.println(response);
        DynamicJsonDocument json(1024);
        deserializeJson(json, response);          
        m_completition = json["choices"][0]["message"]["content"].as<String>();
        
        if (POTO_DEBUG)
        {
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
          Serial.println(requestBody);
          Serial.println(m_completition);
        }
      }
      else if (POTO_DEBUG)
      {
        Serial.print("httpCode: ");
        Serial.println(httpCode);
      }
    }
    else if (POTO_DEBUG)
    {
      Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      Serial.println(prompt);
    }
    https.end();
  }
  else if (POTO_DEBUG)
  {
    Serial.println("[HTTPS] Unable to connect\n");
  }
}

/**
 */
String
Poto::get_post()
{
  return m_completition;
}

/**
 * 
 */
boolean
Poto::min_15()
{
  boolean result = false;
  unsigned long now = millis();
  unsigned long diff = (now > m_clock) ? now - m_clock : m_clock - now;

  if (diff > 900000)
  {
    m_clock = millis();
    result = true;
  }

  return result;
}

// ------------------------------------------------------------------
// PRIVATE
// ------------------------------------------------------------------
/**
*/
void
Poto::init_wifi()
{
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  WiFi.mode(WIFI_STA);

  // Conexión Wi-Fi
  WiFi.begin(m_ssid, m_wifiPsw);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("WiFi conectado.");
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}
/**
*/
void
Poto::init_pins()
{
  pinMode(LDR_PIN, INPUT);
  pinMode(DHTPIN, INPUT);
}
