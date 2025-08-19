#include "../wifi_password.h" //just defines WIFI_SSID and WIFI_PASSWORD
#include <WiFi.h>

#define UDP_PORT 7495
#define BTN_8 27
#define BTN_7 26
#define BTN_6 34
#define BTN_5 39
#define BTN_4 25
#define BTN_3 33
#define BTN_2 32
#define BTN_1 35

struct __attribute__((packed)) advertisement {
	byte mac[6];
};

//====== globals ======
byte mac[6];
WiFiUDP udp;

void setup(){
	Serial.begin(9600);
	WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	WiFi.macAddress(mac);
	Serial.println("Started");
	Serial.printf("connecting to %s\n",WIFI_SSID);
	//while (auto status = WiFi.status() != WL_CONNECTED){
	//	if (status == WL_CONNECT_FAILED){
	//		Serial.printf("connection failed. Trying again...\n");
	//		Serial.printf("connecting to %s\n",WIFI_SSID);
	//		WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	//	}
	//}
	//Serial.println("connected!");
	udp.begin(UDP_PORT);
	//input
	pinMode(BTN_8,INPUT_PULLUP);
	pinMode(BTN_7,INPUT_PULLUP);
	pinMode(BTN_6,INPUT_PULLUP);
	pinMode(BTN_5,INPUT_PULLUP);
	pinMode(BTN_4,INPUT_PULLUP);
	pinMode(BTN_3,INPUT_PULLUP);
	pinMode(BTN_2,INPUT_PULLUP);
	pinMode(BTN_1,INPUT_PULLUP);
}

void loop(){
	uint8_t status = 0
		+(digitalRead(BTN_8) << 7)
		+(digitalRead(BTN_7) << 6)
		+(digitalRead(BTN_6) << 5)
		+(digitalRead(BTN_5) << 4)
		+(digitalRead(BTN_4) << 3)
		+(digitalRead(BTN_3) << 2)
		+(digitalRead(BTN_2) << 1)
		+(digitalRead(BTN_1) << 0);
	Serial.write(status);
	delay(100);
}
