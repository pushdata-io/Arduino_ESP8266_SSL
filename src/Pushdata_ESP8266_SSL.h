#ifndef PUSHDATA_ESP8266_H
#define PUSHDATA_ESP8266_H

#include <Arduino.h>

#define DEBUG
#undef DEBUG

#ifdef DEBUG
#define DBGPRINTHEADER Serial.print("Pushdata_8266_SSL: ")
#define DBGPRINT(X) (Serial.print(X))
#define DBGPRINTLN(X) (Serial.println(X))
#define DBGPRINTH(X) (DBGPRINTHEADER && Serial.print(X))
#define DBGPRINTHLN(X) (DBGPRINTHEADER && Serial.println(X))
#else
#define DBGPRINT(X)
#define DBGPRINTLN(X)
#define DBGPRINTH(X)
#define DBGPRINTHLN(X)
#endif

#ifdef ESP8266
#include "ESP8266WiFi.h"
#include "ESP8266WiFiMulti.h"
#include "WiFiClientSecure.h"
#include "BearSSLHelpers.h"
#include "user_interface.h"

ESP8266WiFiMulti _wifiMulti;

class Pushdata_ESP8266_SSL {
    public:
        void setEmail(const char *emailAddress) {
            strncpy(email, emailAddress, 50);
        }
        void addWiFi(const char *ssid, const char *passwd) {
            _wifiMulti.addAP(ssid, passwd);
        }
        int monitorWiFi() {
            int ret = _wifiMulti.run();
            if (connecting) {
                if (ret == WL_CONNECTED) {
                    DBGPRINTH("Connected to network "); DBGPRINTLN(WiFi.SSID().c_str());
                    connecting = false;
                }
            } else {
                if (ret != WL_CONNECTED) {
                    DBGPRINTHLN("Lost WiFi connection, reconnecting");
                    connecting = true;
                }
            }
            return ret;
        }
        void setApiKey(const char *key) {
            strncpy(apikey, key, 20);
        }
        void setCPUSpeed(unsigned int speed) {
            switch (speed) {
                case 80:
                case 160:
                    cpu_speed = speed;
                    break;
                default:
                    Serial.println("Pushdata_ESP8266_SSL: Error: CPU speed must be 80 or 160");
            }
        }
        // Send data without a TS name, which uses its ethernet MAC address as the name
        int send(float value) {
            int ret = _send(NULL, value, NULL, NULL, 0);
            return ret;
        }
        // int value version (will be converted to float)
        int send(int value) {
            return _send(NULL, (float)value, NULL, NULL, 0);
        }
        // Store data in a specific TS
        int send(const char *tsname, float value) {
            return _send(tsname, value, NULL, NULL, 0);
        }
        // int value version
        int send(const char *tsname, int value) {
            return _send(tsname, (float)value, NULL, NULL, 0);
        }
        // send with timestamp as string, in Unix EPOCH or RFC3336 format
        int send(const char *tsname, float value, const char *timestamp) {
            return _send(tsname, value, timestamp, NULL, 0);
        }
        // and int value version
        int send(const char *tsname, int value, const char *timestamp) {
            return _send(tsname, (float)value, timestamp, NULL, 0);
        }
        // send with timestamp as integer Unix EPOCH 
        int send(const char *tsname, float value, long timestamp) {
            static char ts[12];
            sprintf(ts, "%ld", timestamp);
            return _send(tsname, value, ts, NULL, 0);
        }
        // and int value version
        int send(const char *tsname, int value, long timestamp) {
            static char ts[12];
            sprintf(ts, "%ld", timestamp);
            return _send(tsname, (float)value, ts, NULL, 0);
        }
        // Store data in a specific TS and with tags
        // **tags = { "key1", "val1", "key2", "val2" ... }
        // Use null timestamp and/or tsname to get the defaults
        int send(const char *tsname, float value, const char *timestamp, const char **tags, int numtags) {
            return _send(tsname, value, timestamp, tags, numtags);
        }
        // int value version
        int send(const char *tsname, int value, const char *timestamp, const char **tags, int numtags) {
            return _send(tsname, (float)value, timestamp, tags, numtags);
        }
        // Store data in a specific TS and with tags
        // **tags = { "key1", "val1", "key2", "val2" ... }
        // Use null timestamp and/or tsname to get the defaults
        // integer timestamp version
        int send(const char *tsname, float value, long timestamp, const char **tags, int numtags) {
            static char ts[12];
            sprintf(ts, "%ld", timestamp);
            return _send(tsname, value, ts, tags, numtags);
        }
        // int value version
        int send(const char *tsname, int value, long timestamp, const char **tags, int numtags) {
            static char ts[12];
            sprintf(ts, "%ld", timestamp);
            return _send(tsname, (float)value, ts, tags, numtags);
        }
        // Function that does the actual sending
        int _send(const char *tsname, float value, const char *timestamp, const char **tags, int numtags) {
            static char _tsname[51];
            const char pubkey[] = "-----BEGIN PUBLIC KEY-----\n"
                "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1hXt5g1q0NbHAQwG4w6T\n"
                "w9YtviEJytQjM+fBsMeUEol1d8qOqVgF6aiOthYJM1yKytKQ488tAIXdx7w4dM77\n"
                "lLKLKXjEkeIruVf0L5RRLSMH6NzIAVOpImggQWs5VU+nmmj6TOK7pQNqi9SveJMZ\n"
                "L/XgI10GzjbJU5Bv5sKZr7OY99Ot6DTGyyreK2wLw5GHTuKlh/yRypDiw0G1d9nO\n"
                "E9tNdzCtJzSJJfd/ZZ32fpu0tXzRKyD5lWeR4Qu0qTjCYajZXDxMrBd3DbrROrEf\n"
                "rl0u7L4Hiu6xxENObqS7PEBtdgM3WqnjH6GUc9UNH+1JSnpGOPffpXYHQ0aGUr52\n"
                "FwIDAQAB\n"
                "-----END PUBLIC KEY-----";
            if ((email == NULL || strlen(email) == 0) && (apikey == NULL || strlen(apikey) == 0)) {
                Serial.println("Pushdata_ESP8266_SSL: Error: you must set either an email or an api key");
                return 0;
            }
            if (tsname == NULL) {
                uint8_t mac[6];
                WiFi.macAddress(mac);
                sprintf(_tsname, "---%02x%02x%02x%02x%02x%02x__", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                DBGPRINTH("Sending to TS "); DBGPRINTLN(tsname);
            } else {
                strncpy(_tsname, tsname, 50);
                _tsname[50] = '\0';  // only the paranoid survive
            }

            // Set CPU frequency to either 80 or 160 Mhz
            // 160 is default, unless user has called setCPUSpeed()
            // We do this because SSL handshake time is over 3 seconds at 80 Mhz (~2 secs at 160).
            system_update_cpu_freq(cpu_speed);
            DBGPRINTHLN("BearSSL::WifiClientSecure init");
            BearSSL::WiFiClientSecure client;
            DBGPRINTHLN("BearSSLPublicKey init");
            //
            // Instantiate a BearSSL PublicKey class. 
            BearSSL::PublicKey key;
            // If you're using PlatformIO and encounter problems compiling the above line, try 
            // replacing it with the line below that says "BearSSLPublicKey key;"
            //
            // BearSSLPublicKey key;
            //
            if (key.parse((uint8_t *)pubkey, strlen(pubkey))) {
                DBGPRINTHLN("key.parse() succeeded");
            } else {
                DBGPRINTHLN("key.parse() failed");
                while (1);
            }
            DBGPRINTHLN("client.setKnownKey()");
            client.setKnownKey(&key);
            yield();
            DBGPRINTHLN("client.connect()");
            #ifdef DEBUG
            unsigned long startConnect = millis();
            #endif
            //client.setDefaultNoDelay(true);
            if (!client.connect("pushdata.io", 443)) {
                Serial.println("Pushdata_ESP8266: Error: failed to connect to pushdata.io:443");
                char buf[200];
                int err = client.getLastSSLError(buf, 199);
                buf[199] = '\0';
                Serial.println("Last SSL error was:");
                Serial.println(buf);
                Serial.print("ERRCODE: "); Serial.println(err);
                return 0;
            }
            DBGPRINTH("connect took "); DBGPRINT(millis()-startConnect); DBGPRINTLN(" ms");
            DBGPRINTHLN("Connected to pushdata.io:443");
            static char packetBuf[300];
            memset((void *)packetBuf, 0, 300);
            if (timestamp == NULL) {
                sprintf(packetBuf, "{\"name\":\"%s\",\"points\":[{\"value\":%f}]", _tsname, value);
            } else {
                sprintf(packetBuf, "{\"name\":\"%s\",\"points\":[{\"time\":\"%s\",\"value\":%f}]", _tsname, timestamp, value);
            }

            if (numtags > 0) {
                int l;
                strcat(packetBuf, ",\"tags\":{");
                for (int i = 0; i < numtags; i++) {
                    l = strlen(packetBuf);
                    if ((strlen(tags[i*2])+strlen(tags[i*2+1])+8) >= 250) {
                        Serial.println("Pushdata_ESP8266: Error: packet size exceeded 250 bytes including tags");
                        return 0;
                    }
                    sprintf(packetBuf+l, "\"%s\":\"%s\",", tags[i*2], tags[i*2+1]);
                }
                // remove extraneous comma
                packetBuf[strlen(packetBuf)-1] = '\0';
                // add closing seagull
                strcat(packetBuf, "}");
            }
            // final closing brace
            strcat(packetBuf, "}");
            DBGPRINTH("packetBuf: len="); DBGPRINTLN(strlen(packetBuf)); DBGPRINT("   "); DBGPRINTLN(packetBuf);
            #ifdef DEBUG
                int written = httpPOST(&client, packetBuf);
            #else
                httpPOST(&client, packetBuf);
            #endif
            DBGPRINTH("Wrote "); DBGPRINT(written); DBGPRINTLN(" bytes");


            DBGPRINTHLN("request sent, reading response:");
            // Wait max 2s for response
            unsigned long startRecv = millis();
            client.setTimeout(1000); // should be the default, but only the paranoid survive
            while (client.connected()) {
                yield();
                String line = client.readStringUntil('\n');
                DBGPRINT("   "); DBGPRINTLN(line);
                if (line == "\r\n") {
                    // End of HTTP response encountered
                    break;
                }
                if ((millis() - startRecv) >= 2000) {
                    Serial.println("Pushdata_ESP8266_SSL: timed out reading HTTP response");
                    break;
                }
            }
            client.stop();
            return 1;
        }
    private:
        char apikey[21] = "";
        char email[51] = "";
        bool connecting = false;
        unsigned int cpu_speed = 160;
        int _httpPOST(BearSSL::WiFiClientSecure *client, const char *queryParam, const char *payload) {
            int written = 0;
            written += client->print(String("POST /api/timeseries?") + queryParam + " HTTP/1.1\r\n");
            written += client->print("Host: pushdata.io\r\n");
            written += client->print("User-Agent: Pushdata_ESP8266_SSL\r\n");
            written += client->print("Content-Type: application/json\r\n");
            written += client->print(String("Content-Length: ") + strlen(payload) + "\r\n");
            written += client->print("Connection: close\r\n\r\n");
            written += client->println(payload);
            return written;
        }
        int httpPOST(BearSSL::WiFiClientSecure *client, const char *payload) {
            if (apikey != NULL && strlen(apikey) > 0) {
                return _httpPOST(client, (String("apikey=") + apikey).c_str(), payload);
            }
            if (email != NULL && strlen(email) > 0) {
                return _httpPOST(client, (String("email=") + email).c_str(), payload);
            }
            return 0;
        }

};

#endif

#endif
