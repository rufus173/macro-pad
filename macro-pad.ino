#include "../wifi_password.h" //just defines WIFI_SSID and WIFI_PASSWORD
#include <WiFi.h>

#define UDP_PORT 7495

struct __attribute__((packed)) advertisement {
	byte mac[6];
}

//====== globals ======
byte mac[6];
WiFiUDP udp;

void setup(){
	Serial.begin(9600);
	WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	Wifi.macAddress(mac);
	Serial.println("Started");
	Serial.printf("connecting to %s\n",WIFI_SSID);
	while (auto status = WiFi.status() != WL_CONNECTED){
		if (status == WL_CONNECT_FAILED){
			Serial.printf("connection failed. Trying again...\n");
			Serial.printf("connecting to %s\n",WIFI_SSID);
			WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
		}
	}
	Serial.println("connected!");
	udp.begin(UDP_PORT);
}

void loop(){

}
