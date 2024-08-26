#include <WiFi.h>
#include <MQTT.h>

WiFiClient net;
MQTTClient client;

const char ssid[] = "vivo 1902";
const char pass[] = "lakasandie";

// Pin untuk relay
const int relay1Pin = 16;
const int relay2Pin = 17;
const int relay3Pin = 18;
const int relay4Pin = 19;

String serial_number = "87654321";
bool isWiFiConnected = false;

void setup() {
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);
  pinMode(relay4Pin, OUTPUT);

  // Atur status awal relay (mati)
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);
  digitalWrite(relay3Pin, HIGH);
  digitalWrite(relay4Pin, HIGH);

  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  client.begin("broker.emqx.io", net);

  client.onMessage(messageReceived);
  connectToWiFiAndMQTT();
}

void loop() {
  isWiFiConnected = WiFi.status() == WL_CONNECTED;

  // Coba reconnect jika WiFi terputus
  if (isWiFiConnected && !client.connected()) {
    connectToWiFiAndMQTT();
  }

  if (isWiFiConnected) {
    client.loop();
  }
}

void messageReceived(String &topic, String &payload) {
  if (isWiFiConnected) {  // Hanya memproses pesan MQTT jika online
    Serial.print("Pesan diterima di topik: ");
    Serial.println(topic);
    Serial.print("Payload: ");
    Serial.println(payload);
    if (payload == "on" || payload == "off") {
      if (topic == "tobrut/87654321/relay1") {
        controlRelay(1, payload == "on");
      } else if (topic == "tobrut/87654321/relay2") {
        controlRelay(2, payload == "on");
      } else if (topic == "tobrut/87654321/relay3") {
        controlRelay(3, payload == "on");
      } else if (topic == "tobrut/87654321/relay4") {
        controlRelay(4, payload == "on");
      }
    } else {
      Serial.println("Invalid payload received: " + payload);
    }
  }
}

void connectToWiFiAndMQTT() { 
  Serial.println("Menghubungkan ke WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Terhubung ke WiFi");

  client.setWill("tobrut/status/87654321" , "offline", true, 1);
  while (!client.connect("tobrut")) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("Terhubung ke Broker");
  client.publish("tobrut/status/87654321" , "online", true, 1);
  client.subscribe("tobrut/#", 1);

  // Kirim status relay terbaru setelah terhubung
  sendRelayStatus(1);
  sendRelayStatus(2);
  sendRelayStatus(3);
  sendRelayStatus(4);
}

void controlRelay(int relayNumber, bool turnOn) {
  int relayPin;
  
  switch (relayNumber) {
    case 1:
      relayPin = relay1Pin;
      break;
    case 2:
      relayPin = relay2Pin;
      break;
    case 3:
      relayPin = relay3Pin;
      break;
    case 4:
      relayPin = relay4Pin;
      break;
    default:
      return;
  }

  digitalWrite(relayPin, turnOn ? LOW : HIGH);

  const char* status = turnOn ? "on" : "off";

  Serial.print("Relay ");
  Serial.print(relayNumber);
  Serial.print(" status: ");
  Serial.println(status);

  // Mengirim status relay ke MQTT jika terhubung
  if (isWiFiConnected && client.connected()) {
    String topic = "tobrut/87654321/relay" + String(relayNumber);
    Serial.print("Mengirim topik: ");
    Serial.println(topic);
    Serial.print("Mengirim payload: ");
    Serial.println(status);
    client.publish(topic.c_str(), status, true);
  }
}

void sendRelayStatus(int relayNumber) {
  String topic = "tobrut/87654321/relay" + String(relayNumber);
  String status = digitalRead((relayNumber == 1) ? relay1Pin :
                               (relayNumber == 2) ? relay2Pin :
                               (relayNumber == 3) ? relay3Pin : relay4Pin) == LOW ? "on" : "off";
  client.publish(topic.c_str(), status.c_str(), true);
}
