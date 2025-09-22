#include "../wifi_password.h" //just defines WIFI_SSID and WIFI_PASSWORD
#include <WiFi.h>
#include <LiquidCrystal.h>

#define UDP_PORT 7495
#define BTN_8 27
#define BTN_7 26
#define BTN_6 34
#define BTN_5 39
#define BTN_4 25
#define BTN_3 33
#define BTN_2 32
#define BTN_1 35
#define LCD_RS 21
#define LCD_RW 19
#define LCD_E 18
#define LCD_D7 15
#define LCD_D6 2
#define LCD_D5 4
#define LCD_D4 5

struct __attribute__((packed)) advertisement {
	byte mac[6];
	uint8_t state;
};
struct __attribute__((packed)) serial_data_header {
	uint8_t type;
	uint8_t length;
};

//====== globals ======
byte mac[6];
WiFiUDP udp;
LiquidCrystal lcd(LCD_RS,LCD_RW,LCD_E,LCD_D4,LCD_D5,LCD_D6,LCD_D7);

void setup(){
	lcd.begin(16,2);
	lcd.print("Online");
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
	//udp.begin();
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
	//====== read and send the button status ======
	static uint8_t prev_status = 0;
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
	if (status != prev_status){
		udp.beginPacket(IPAddress(255,255,255,255),UDP_PORT);
		struct advertisement packet = {0};
		memcpy(packet.mac,mac,sizeof(mac));
		packet.state = status;
		udp.write((const uint8_t *)&packet,sizeof(struct advertisement));
		udp.endPacket();
	}
	prev_status = status;
	//====== read commands and data from serial ======
	while (Serial.available() >= 2){
		struct serial_data_header header;
		int bytes_read = Serial.read((uint8_t *)&header,sizeof(struct serial_data_header));
		//lcd.clear();
		//lcd.print("read "+String(bytes_read)+" bytes");
		if (bytes_read == sizeof(struct serial_data_header)){
			char buffer[UINT8_MAX+2];
			memset(buffer,0,sizeof(buffer));
			int bytes_read = Serial.readBytes(buffer,header.length);
			//String debug = String(bytes_read) + " " + String(header.length);
			//lcd.clear();
			//lcd.print(String(header.type) + " " + String(header.length));
			switch (header.type){
			case 0: //====== write data to lcd screen row 0======
				lcd.print(buffer);
				break;
			case 1:
				lcd.setCursor(0,1);
				lcd.print(buffer);
				break;
			case 2:
				lcd.clear();
				break;
			}
		}
	}
	delay(50);
}
