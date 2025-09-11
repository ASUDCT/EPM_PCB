/*
    ESP-NOW Broadcast Slave
    Lucas Saavedra Vaz - 2024

    This sketch demonstrates how to receive broadcast messages from a master device using the ESP-NOW protocol.

    The master device will broadcast a message every 5 seconds to all devices within the network.

    The slave devices will receive the broadcasted messages. If they are not from a known master, they will be registered as a new master
    using a callback function.
*/

#include "ESP32_NOW.h"
#include "WiFi.h"

#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

#include <vector>

/* Definitions */

#define ESPNOW_WIFI_CHANNEL 6

#define SLAVE_ID 2

#define RGB_BRIGHTNESS 64 // Change white brightness (max 255)

// the setup function runs once when you press reset or power the board*9
#define PIN_NEOPIXEL 38

void LEDblink(int num){
  neopixelWrite(PIN_NEOPIXEL, 0, 0, 0);
  delay(50);
  if(num==1)neopixelWrite(PIN_NEOPIXEL, 0, RGB_BRIGHTNESS, 0);  // Red
  else if(num==2)neopixelWrite(PIN_NEOPIXEL, 0.2*RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0); // Orange
  else if(num==3)neopixelWrite(PIN_NEOPIXEL, RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0);  // Yellow
  else if(num==4)neopixelWrite(PIN_NEOPIXEL, RGB_BRIGHTNESS, 0, 0);  // Green
  else if(num==5)neopixelWrite(PIN_NEOPIXEL, 0, 0, RGB_BRIGHTNESS);  // blue
  else if(num==6)neopixelWrite(PIN_NEOPIXEL, RGB_BRIGHTNESS, 0, RGB_BRIGHTNESS);  // indigo
  else if(num==7)neopixelWrite(PIN_NEOPIXEL, 0, 0.5*RGB_BRIGHTNESS, RGB_BRIGHTNESS); // violet
  else if(num==8)neopixelWrite(PIN_NEOPIXEL, RGB_BRIGHTNESS, RGB_BRIGHTNESS, RGB_BRIGHTNESS);  // White
  delay(500);
}

void EPMout0(int num){
  digitalWrite(2*num-1, LOW);
  digitalWrite(2*num, LOW);
  digitalWrite(2*num-1, HIGH);
  // analogWrite(2*num-1,255);
  delay(3);
  digitalWrite(2*num-1, LOW);
  // analogWrite(2*num-1,0);
  LEDblink(num);
}

void EPMout1(int num){
  digitalWrite(2*num-1, LOW);
  digitalWrite(2*num, LOW);
  digitalWrite(2*num, HIGH);
  // analogWrite(2*num,255);
  delay(3);
  digitalWrite(2*num, LOW);
  // analogWrite(2*num,0);
  LEDblink(num);
  LEDblink(num);
}

/* Classes */

// Creating a new class that inherits from the ESP_NOW_Peer class is required.

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  // Constructor of the class
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk) : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  // Destructor of the class
  ~ESP_NOW_Peer_Class() {}

  // Function to register the master peer
  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }
  // Function to print the received messages from the master
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    // Serial.printf("Received a message from master " MACSTR " (%s)\n", MAC2STR(addr()), broadcast ? "broadcast" : "unicast");
    // Serial.printf("  Message: %s\n", (char *)data);
    String data_str;
    data_str= String((char *)data);

    if(data_str.length()<7){
      String id_char = data_str.substring(0,2);
      String epm_char= data_str.substring(2,4);
      String epm_dir_char= data_str.substring(4,5);
      int id, epm_id, epm_dir;
      id = id_char.toInt();
      epm_id = epm_char.toInt();
      epm_dir = epm_dir_char.toInt();
      
      // delay(100);
      // Serial.println(String(id));
      // delay(100);
      // Serial.println(String(epm_id));
      // delay(100); 
      // Serial.println(String(epm_dir));
      // delay(100); 

      if(id==SLAVE_ID){
        if(epm_id<9&epm_id>0){
          if(epm_dir==0)EPMout0(epm_id);
          else if(epm_dir==1)EPMout1(epm_id);
          else Serial.printf("epm_dir error");
        }
        else Serial.printf("epm_id error");
        }
      else Serial.printf("SLAVE_ID error");
      }
  }
};

/* Global Variables */

// List of all the masters. It will be populated when a new master is registered
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */

// Callback called when an unknown peer sends a message
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");

    ESP_NOW_Peer_Class new_master(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);

    masters.push_back(new_master);
    if (!masters.back().add_peer()) {
      Serial.println("Failed to register the new master");
      return;
    }
  } else {
    // The slave will only receive broadcast messages
    log_v("Received a unicast message from " MACSTR, MAC2STR(info->src_addr));
    log_v("Igorning the message");
  }
}

/* Main */

void setup() {

  

  // GPIO init
  for(int i=1;i<17;i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  neopixelWrite(PIN_NEOPIXEL, 0, 0, 0); 
  neopixelWrite(PIN_NEOPIXEL, 0.4*RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0.7*RGB_BRIGHTNESS);

  // Serial init
  Serial.begin(115200);
  // while (!Serial) {
  //   delay(10);
  // }

  // Initialize the Wi-Fi module
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.println("ESP-NOW Example - Broadcast Slave");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);
  Serial.printf("  ID: %d\n", SLAVE_ID);

  // Initialize the ESP-NOW protocol
  if (!ESP_NOW.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    Serial.println("Reeboting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }

  // Register the new peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);

  Serial.println("Setup complete. Waiting for a master to broadcast a message...");
}

void loop() {
  neopixelWrite(PIN_NEOPIXEL, 0.4*RGB_BRIGHTNESS, RGB_BRIGHTNESS, 0.7*RGB_BRIGHTNESS);
  delay(1000);
}
