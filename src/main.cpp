#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
<<<<<<< HEAD
#include "credentials.h"
=======
#include <WiFiClientSecure.h>
#include "flight_info.h"
#include "credentials.h"

>>>>>>> 898f137 (Get now recieves icao24, callsign, and plane model (which is gotten from an api hosted on cloudfare containing an up to date tar1090-db))
#define token_buffer_size 2048

void get_info(flight_info* info);
void get_token(char* token_buffer);
void to_uppercase(char* upper_str, const char* str);
char token_buffer[token_buffer_size];

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(921600);
    delay(3000);

    Serial.print("Attempting Wifi connection.");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }

    Serial.print("\nConnected to Wifi with IP: ");
    Serial.println(WiFi.localIP());
    get_token(token_buffer);
}

void loop() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    flight_info info;
    get_info(&info);
    if (info.icao24[0] != '\0') {
        Serial.println(info.icao24);
        if (info.callsign[0] != '\0') {
            Serial.println(info.callsign);
        }
        Serial.println(info.model);
    }
    delay(5000);
}

void get_info(flight_info* info) {
    JsonDocument data_json;
    const char* icao24 = "";
    HTTPClient http;
    http.useHTTP10(true);
    http.begin("https://opensky-network.org/api/states/all?lamin=50&lomin=-105&lamax=60&lomax=-100");
    http.setAuthorizationType("Bearer");
    http.setAuthorization(token_buffer);
    int http_response_code = http.GET();
    Serial.print("Data fetch response code = ");
    Serial.println(http_response_code);
    if (http_response_code < 0) {
        Serial.println("Request failed");
    } else if (http_response_code == 401) { //Get new token
        http.end();
        get_token(token_buffer);
        http.begin("https://opensky-network.org/api/states/all?lamin=50&lomin=-105&lamax=60&lomax=-100");
        http.useHTTP10(true);
        http.setAuthorizationType("Bearer");
        http.setAuthorization(token_buffer);
        http_response_code = http.GET();
    }
    DeserializationError error = deserializeJson(data_json, http.getStream());
    http.end();
    if (error) {
        Serial.print("Data deserialization failed: ");
        Serial.println(error.c_str());
        return;
    } else {
        JsonArray states = data_json["states"].as<JsonArray>();
        if (states.size() == 0) {
            Serial.println("States array empty");
            info->icao24[0] = '\0';
            info->callsign[0] = '\0';
            return;
        }

        JsonArray first_state = states[0].as<JsonArray>();
        if (first_state.size() == 0) {
            Serial.println("First state vector empty");
            info->icao24[0] = '\0';
            info->callsign[0] = '\0';
            return;
        }

        const char* icao24 = first_state[0]; // index 0 is icao24 
        if (icao24 == nullptr) {
            Serial.println("icao24 is null");
            info->icao24[0] = '\0';
            return;
        }

        const char* callsign = first_state[1]; // index 1 is callsign 
        if (icao24 == nullptr) {
            Serial.println("callsign is null");
            info->callsign[0] = '\0';
            return;
        }
        strncpy(info->callsign, callsign, 8); //Callsigns are 8 characters
        strncpy(info->icao24, icao24, 6); //icao24's are 6 characters
        info->callsign[8] = '\0';
        info->icao24[6] = '\0';
        
        char url[64] = "http://cfapi.schaturvedi.workers.dev/api/";
        char icao_uppercase[7];
        to_uppercase(icao_uppercase, icao24);
        icao_uppercase[6] = '\0';
        strncat(url, icao_uppercase, 6);
        Serial.println(url);
        HTTPClient http2;
        http2.begin(url);
        http_response_code = http2.GET();
        if (http_response_code < 0) {
            Serial.println("Model Request failed");
        } else {
            WiFiClient* stream = http2.getStreamPtr();
            if (http2.getSize() > 0 && stream->available()) {
                size_t len = http2.getSize();
                size_t read_len = stream->readBytes(info->model, min(len, sizeof(info->model) - 1));
                info->model[read_len] = '\0';
            }
        http2.end();
        }
    }
}

void get_token(char* token_buffer) {
    const char* token = nullptr;
    JsonDocument token_json;
    HTTPClient http;
    Serial.println("Retrieving Token");

    http.useHTTP10(true);
    http.begin("https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String post_data;
    post_data.reserve(256);  // Preallocate enough space to avoid fragmentation (using String is unavoidable here)
    post_data = "grant_type=client_credentials&client_id=";
    post_data += CLIENT_ID;
    post_data += "&client_secret=";
    post_data += CLIENT_SECRET;
    int http_response_code = http.POST(post_data);
    if (http_response_code < 0) {
        Serial.println("Request failed");
    }
    DeserializationError error = deserializeJson(token_json, http.getStream());
    http.end();
    if (error) {
    Serial.print("Token deserialization failed: ");
    Serial.println(error.c_str());
    return;
    } else {
        token = token_json["access_token"].as<const char*>();
        strlcpy(token_buffer, token, token_buffer_size);
    }
}

void to_uppercase(char* upper_str, const char* str) {
    while (*str) {
        *upper_str = toupper(*str);
        str++;
        upper_str++;
    }
}