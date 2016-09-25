#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <SoftTimer.h>

const char ssid[] = "hubwestminster";
const char pass[] = "HubWest1";

const unsigned short BROADCAST_PORT = 8980;
const unsigned short HTTP_PORT = 80;
const unsigned short HTTPS_PORT = 443;

const int POWER0_CHARGING = 4;
const int POWER0_WASTING = 5;

const int POWER1_CHARGING = 14;
const int POWER1_WASTING = 12;

const char WORLDPAY_HOST[] = "api.worldpay.com";
//const char WORLDPAY_FINGERPRINT[] = "69 6E 5E 77 D2 E2 69 96 CD 1E 59 5A BF 09 36 5D 81 EB CA 17";
const char MERCHANT_SERVICE_KEY[] = "T_S_15a8b01b-0356-4085-b901-95ed869c7297";
const char MERCHANT_CLIENT_KEY[] = "T_C_f9f19867-3ca5-48b9-8a6b-3202230219b9";

uint32_t power0Mean;
uint32_t power1Mean;
uint32_t power2Mean;
uint16_t power0Vals[16];
uint16_t power1Vals[16];
uint16_t power2Vals[16];
int power0Price;
int power1Price;
int power2Price;
String power0Description;
String power1Description;
String power2Description;
int powerIndex;
Adafruit_ADS1115 ads;
Task powerTask(0, NULL);

int power0Charging = 0;
int power1Charging = 0;

String serverId;
WiFiUDP udp;
Task udpTask(0, NULL);
ESP8266WebServer server(HTTP_PORT);
Task webServerTask(0, NULL);
WiFiClientSecure secureClient;

int nextPaymentReferenceId = 0;
int paymentReferencePriceId[16];
int paymentReferenceNumberUnits[16];
int paymentReferencePricePerUnit[16];

void updateConditions() {
  String description;
  bool powerOff = false;
  if (power0Mean < 3200) {
    description = "Dead calm";
    power0Price = 10;
    powerOff = true;
  } else if (power0Mean < 12800) {
    description = "Breezy";
    power0Price = 6;
  } else {
    description = "Hurricane";
    power0Price = 3;
  }
  power0Description = String(description + " - " + String(power0Mean >> 6) + "mV");
  if (power0Charging > 0) {
    digitalWrite(POWER0_CHARGING, HIGH);
    digitalWrite(POWER0_WASTING, LOW);    
    power0Charging--;
  } else if (powerOff) {
    digitalWrite(POWER0_CHARGING, LOW);
    digitalWrite(POWER0_WASTING, LOW);
  } else {
    digitalWrite(POWER0_CHARGING, LOW);
    digitalWrite(POWER0_WASTING, HIGH);
  }
  powerOff = false;
  if (power1Mean < 64000) {
    description = "Night";
    power1Price = 10;
    powerOff = true;
  } else if (power1Mean < 112000) {
    description = "Overcast";
    power1Price = 4;
  } else {
    description = "Bright sun";
    power1Price = 2;
  }
  if (power1Charging > 0) {
    digitalWrite(POWER1_CHARGING, HIGH);
    digitalWrite(POWER1_WASTING, LOW);    
    power1Charging--;
  } else if (powerOff) {
    digitalWrite(POWER1_CHARGING, LOW);
    digitalWrite(POWER1_WASTING, LOW);
  } else {
    digitalWrite(POWER1_CHARGING, LOW);
    digitalWrite(POWER1_WASTING, HIGH);
  }
  power1Description = String(description + " - " + String(power1Mean >> 6) + "mV");
  if (power2Mean < 64000) {
    description = "Night";
    power2Price = 10;
  } else if (power2Mean < 112000) {
    description = "Overcast";
    power2Price = 4;
  } else {
    description = "Bright sun";
    power2Price = 2;
  }
  power2Description = String(description + " - " + String(power2Mean >> 6) + "mV");
}

void setup()
{
  Serial.begin(9600);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  serverId = "12345678-1234-1234-1234-123456789012"; // TODO generate uniquely
  ads.begin();
  Wire.begin(0, 2);

  pinMode(POWER0_CHARGING, OUTPUT);
  pinMode(POWER0_WASTING, OUTPUT);

  pinMode(POWER1_CHARGING, OUTPUT);
  pinMode(POWER1_WASTING, OUTPUT);

  uint16_t reading0 = ads.readADC_SingleEnded(0);
  uint16_t reading1 = ads.readADC_SingleEnded(1);
  uint16_t reading2 = ads.readADC_SingleEnded(2);
  if (reading0 > 65000) {
    // Turbine is often 0V, and sometimes drops marginally below - don't wrap!
    reading0 = 0;
  }
  power0Mean = reading0 * 16;
  power1Mean = reading1 * 16;
  power2Mean = reading2 * 16;
  for (int ii = 0; ii < 16; ii++) {
    power0Vals[ii] = reading0;
    power1Vals[ii] = reading1;
    power2Vals[ii] = reading2;
  }
  updateConditions();

  powerTask = Task(250, [](Task* me) {
    uint16_t reading0 = ads.readADC_SingleEnded(0);
    uint16_t reading1 = ads.readADC_SingleEnded(1);
    uint16_t reading2 = ads.readADC_SingleEnded(2);
    if (reading0 > 65000) {
      // Turbine is often 0V, and sometimes drops marginally below - don't wrap!
      reading0 = 0;
    }
    Serial.print(reading0);
    Serial.print("\t");
    Serial.print(reading1);
    Serial.print("\t");
    Serial.print(reading2);
    Serial.print("\t");
    power0Mean += reading0 - power0Vals[powerIndex];
    power1Mean += reading1 - power1Vals[powerIndex];
    power2Mean += reading2 - power2Vals[powerIndex];
    power0Vals[powerIndex] = reading0;
    power1Vals[powerIndex] = reading1;
    power2Vals[powerIndex] = reading2;
    powerIndex = ((powerIndex + 1) % 16);
    Serial.print(power0Mean / 16);
    Serial.print("\t");
    Serial.print(power1Mean / 16);
    Serial.print("\t");
    Serial.print(power2Mean / 16);
    Serial.println("");
    updateConditions();
  });
  SoftTimer.add(&powerTask);

  udp.begin(BROADCAST_PORT + 1);
  udpTask = Task(1000, [](Task* me) {
    Serial.println("Sending UDP broadcast");

    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    root["deviceDescription"] = "ParetoPower";
    IPAddress localIp = WiFi.localIP();
    root["hostname"] = String(String(localIp[0]) + "." + String(localIp[1]) + "." + String(localIp[2]) + "." + String(localIp[3]));
    root["portNumber"] = HTTP_PORT;
    root["serverID"] = serverId;
    root["urlPrefix"] = "";
    root["scheme"] = "http://";

    IPAddress broadcastIp;
    broadcastIp = ~WiFi.subnetMask() | WiFi.gatewayIP();
    udp.beginPacket(broadcastIp, BROADCAST_PORT);
    root.printTo(udp);
    udp.endPacket();
  });
  SoftTimer.add(&udpTask);

  server.on("/service/discover", []() {
    Serial.println("Handling /service/discover");

    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    root["serverID"] = serverId;
    JsonArray& services = root.createNestedArray("services");
    JsonObject& service = services.createNestedObject();
    service["ServiceID"] = 0;
    service["ServiceName"] = "ParetoPower Charging";
    service["ServiceDescription"] = "Buy sustainably-generated energy from your neighbors!";
    
    char buffer[256];
    root.printTo(buffer, sizeof(buffer));
    server.send(200, "application/json", buffer);
  });

  server.on("/service/0/prices", [](){
    Serial.println("Handling /service/0/prices");

    StaticJsonBuffer<768> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    root["serverID"] = serverId;
    JsonArray& services = root.createNestedArray("prices");
    JsonObject& service = services.createNestedObject();
    service["priceID"] = 0;
    service["priceDescription"] = String(String("Wind energy (") + power0Description + ")");
    JsonObject& pricePerUnit = service.createNestedObject("pricePerUnit");
    pricePerUnit["amount"] = power0Price;
    pricePerUnit["currencyCode"] = "GBP";
    service["unitID"] = 0;
    service["unitDescription"] = "kWh";
    JsonObject& service2 = services.createNestedObject();
    service2["priceID"] = 0;
    service2["priceDescription"] = String(String("Solar energy (") + power1Description + ")");
    JsonObject& pricePerUnit2 = service2.createNestedObject("pricePerUnit");
    pricePerUnit2["amount"] = power1Price;
    pricePerUnit2["currencyCode"] = "GBP";
    service2["unitID"] = 0;
    service2["unitDescription"] = "kWh";
    
    char buffer[768];
    root.printTo(buffer, sizeof(buffer));
    server.send(200, "application/json", buffer);
  });

  server.on("/service/0/requestTotal", [](){
    Serial.println("Handling /service/requestTotal");

    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    String clientId = root["clientID"];
    int selectedPriceId = root["selectedPriceID"];
    int pricePerUnit = 0;
    switch (selectedPriceId) {
      case 0:
        pricePerUnit = power0Price;
        break;
      case 1:
        pricePerUnit = power1Price;
        break;
    }
    int selectedNumberOfUnits = root["selectedNumberOfUnits"];

    paymentReferencePriceId[nextPaymentReferenceId] = selectedPriceId;
    paymentReferenceNumberUnits[nextPaymentReferenceId] = selectedNumberOfUnits;
    paymentReferencePricePerUnit[nextPaymentReferenceId] = pricePerUnit;

    StaticJsonBuffer<300> jsonBuffer2;
    JsonObject& root2 = jsonBuffer2.createObject();
    root2["serverID"] = serverId;
    root2["clientID"] = clientId;
    root2["priceID"] = selectedPriceId;
    root2["unitsToSupply"] = selectedNumberOfUnits;
    root2["totalPrice"] = selectedNumberOfUnits * pricePerUnit;
    root2["currencyCode"] = "GBP";
    root2["paymentReferenceId"] = String(nextPaymentReferenceId); // TODO Generate securely
    root2["merchantClientKey"] = MERCHANT_CLIENT_KEY;

    nextPaymentReferenceId = ((nextPaymentReferenceId + 1) % 16);
    
    char buffer[300];
    root2.printTo(buffer, sizeof(buffer));
    server.send(200, "application/json", buffer);
  });

  server.on("/payment", [](){
    Serial.println("Handling /payment");

    StaticJsonBuffer<384> jsonBuffer;

    Serial.println(server.arg("plain"));

    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    String clientId = root["clientID"];
    String clientToken = root["clientToken"];
    int paymentReferenceId = root.get("paymentReferenceID").as<String>().toInt();
    
    Serial.print("Connecting to ");
    Serial.println(WORLDPAY_HOST);
    if (!secureClient.connect(WORLDPAY_HOST, HTTPS_PORT)) {
      server.send(502, "application/json", "{}");
      return;
    }

    Serial.println("Connected!");
    yield();

//    if (secureClient.verify(WORLDPAY_FINGERPRINT, WORLDPAY_HOST)) {
//      server.send(502, "application/json", "{}");
//      return;
//    }

    secureClient.print(String("POST ") + "/v1/orders" + " HTTP/1.1\r\n" +
                      "Host: " + WORLDPAY_HOST + "\r\n" +
                      "Connection: close\r\n" +
                      "Authorization: " + MERCHANT_SERVICE_KEY + "\r\n" +
                      "Content-Type: application/json\r\n");
    {
      StaticJsonBuffer<512> jsonBuffer2;
      JsonObject& root2 = jsonBuffer2.createObject();
      root2["amount"] = paymentReferenceNumberUnits[paymentReferenceId] * paymentReferencePricePerUnit[paymentReferenceId];
      root2["currencyCode"] = "GBP";
      root2["customerOrderCode"] = "ParetoPower charge";
      root2["orderDescription"] = "Car charging payment";
      root2["token"] = clientToken;
      secureClient.print(String("Content-Length: ") + root2.measureLength() + "\r\n\r\n");
      root2.printTo(Serial);
      root2.printTo(secureClient);
    }

    while (secureClient.connected()) {
      String line = secureClient.readStringUntil('\n');
      yield();
      Serial.println(line);
//      if (line == "\r") {
//        Serial.println("headers received");
//        break;
//      }
    }

    // TODO Check response!!!

    {
      StaticJsonBuffer<384> jsonBuffer2;
      JsonObject& root2 = jsonBuffer2.createObject();
      root2["serverID"] = serverId;
      root2["clientID"] = clientId;
      root2["totalPaid"] = paymentReferenceNumberUnits[paymentReferenceId] * paymentReferencePricePerUnit[paymentReferenceId];
      JsonObject& token = root2.createNestedObject("serviceDeliveryToken");
      token["key"] = String(paymentReferenceId);
      token["issued"] = "2016-09-25T15:04:05Z";
      token["expiry"] = "2016-09-25T16:04:05Z";
      token["refundOnExpiry"] = false;
      token["signature"] = ""; // TODO: Implement signature
    
      char buffer[384];
      root2.printTo(buffer, sizeof(buffer));
      server.send(200, "application/json", buffer);
    }
  });

  server.on("/service/0/delivery/begin", [](){
    Serial.println("Handling /service/0/delivery/begin");

    StaticJsonBuffer<512> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    String clientId = root["clientID"];
    JsonObject& serviceDeliveryToken = root["serviceDeliveryToken"];
    int paymentReferenceId = serviceDeliveryToken.get("key").as<String>().toInt();
    int unitsToSupply= root["unitsToSupply"];

    StaticJsonBuffer<512> jsonBuffer2;
    JsonObject& root2 = jsonBuffer2.createObject();
    root2["serverID"] = serverId;
    root2["clientID"] = clientId;
    root2["serviceDeliveryToken"] = serviceDeliveryToken;
    root2["unitsToSupply"] = unitsToSupply;

    switch (paymentReferencePriceId[paymentReferenceId]) {
      case 0:
        power0Charging += unitsToSupply * 4;
        break;
      case 1:
        power1Charging += unitsToSupply * 4;
        break;
    }
 
    char buffer[512];
    root2.printTo(buffer, sizeof(buffer));
    server.send(200, "application/json", buffer);
  });

  server.on("/service/0/delivery/end", [](){
    Serial.println("Handling /service/0/delivery/end");

    StaticJsonBuffer<512> jsonBuffer;

    JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
    String clientId = root["clientID"];
    JsonObject& serviceDeliveryToken = root["serviceDeliveryToken"];
    int paymentReferenceId = serviceDeliveryToken.get("key").as<String>().toInt();
    int unitsReceived = root["unitsReceived"];

    StaticJsonBuffer<512> jsonBuffer2;
    JsonObject& root2 = jsonBuffer2.createObject();
    root2["serverID"] = serverId;
    root2["clientID"] = clientId;
    root2["serviceDeliveryToken"] = serviceDeliveryToken;
    root2["unitsJustSupplied"] = unitsReceived; // TODO Check unitsReceived was correct
    root2["unitsRemaining"] = paymentReferenceNumberUnits[paymentReferenceId] - unitsReceived;

    switch (paymentReferencePriceId[paymentReferenceId]) {
      case 0:
        power0Charging = 0;
        break;
      case 1:
        power1Charging = 0;
        break;
    }
    
    char buffer[512];
    root2.printTo(buffer, sizeof(buffer));
    server.send(200, "application/json", buffer);
  });

  server.begin();

  webServerTask = Task(10, [](Task* me) {
    server.handleClient();
  });
  SoftTimer.add(&webServerTask);
}
