#include <Ethernet.h>
#include <EthernetUdp.h>   // pin 13,12,11,10 and 4 can NOT be used; they are used by the WIFI shield 
#define relayOnder 7   // used in the future for relay valve
#define relayBoven 8   // used in the future for relay valve
#define inputRegenput A0  // analogue input for sensor (third wire)
#define DEBUG true

#define localPort 1235  // local port to listen on for incoming UDP messages 
#define sendPort 1234  // receiver's port to  send UDP messages to

unsigned long lastPing = 0; // hold timer for ping response
char packetBuffer[255];          // buffer to hold incoming packet

int vorigeWaterstand;
unsigned long lastCheck = 0; // hold timer for checking water values

EthernetUDP Udp;
EthernetClient client; 
IPAddress ip(192, 168, 15, 49);
IPAddress gateway(192, 168, 15, 1);
IPAddress subnet(255, 255, 255, 0);
byte mac[] = {0xAE, 0xDC, 0xDE, 0xEA, 0xBE, 0xAD};

void setup() {
  Serial.begin(9600);
  setethernet();
  pinMode(relayBoven,OUTPUT);
  pinMode(relayOnder,OUTPUT);
  pinMode(inputRegenput,INPUT_PULLUP);
  delay(200);
  sendUDPmessage("REGENPUT:live!!");
  valveBovenclose();
  valveOnderclose();
}

void loop() {
  if (!checkping()) {
    setethernet();
  }

  char* returnboodschap = getUDPmessage();
  if (DEBUG) {
    if (returnboodschap != "") {
      Serial.print(F("volgende boodschap kwam binnen: "));
      Serial.println(returnboodschap);
    }
  }
  
  if (strcmp(returnboodschap, "SEND=valveBoven:open") == 0) {
    valveBovenopen();
  }
  if (strcmp(returnboodschap, "SEND=valveBoven:close") == 0) {
    valveBovenclose();
  }
  if (strcmp(returnboodschap, "SEND=valveOnder:open") == 0) {
    valveOnderopen();
  }
  if (strcmp(returnboodschap, "SEND=valveOnder:close") == 0) {
    valveOnderclose();
  }
  if (strcmp(returnboodschap, "REQUEST=level") == 0) {
    int waterstand = analogRead(inputRegenput);
    char totalmessage[30] = {0};
    char temp[10] = {0};
    sprintf(totalmessage, "REPLY=level:%s", dtostrf(waterstand, 5, 0, temp));
    sendUDPmessage(totalmessage);
    vorigeWaterstand = waterstand;
  }
  if (millis() - lastCheck > 60000) { // check every min
    lastCheck = millis();
    int waterstand = analogRead(inputRegenput);
    if (vorigeWaterstand > (waterstand + 3)|| vorigeWaterstand < (waterstand - 3)){
      char totalmessage[30] = {0};
      char temp[10] = {0};
      sprintf(totalmessage, "REPLY=level:%s", dtostrf(waterstand, 5, 0, temp));
      sendUDPmessage(totalmessage);
      vorigeWaterstand = waterstand;
    }
  }
}

void sendUDPmessage(char* message) {
  if (DEBUG) {
    Serial.println(F("we will now send a message"));
  }
  Udp.beginPacket(IPAddress(192, 168, 15, 39), sendPort);
  uint8_t countBeforeReset = 0;
  while (Udp.write(message) == 0) {
    if (countBeforeReset == 3) {
      setethernet();
      Udp.write(message);
      Serial.println(message);
      break;
    }
    if (DEBUG) {
      Serial.println(F("reconnecting to udp"));
    }
    setudp();
    countBeforeReset ++;
  }
  Serial.print("the counter was");Serial.println(countBeforeReset);
  Serial.print("following message has been written: "); Serial.println(message);
  Udp.endPacket();
  delay(100);
}

char* getUDPmessage() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    if (DEBUG) {
      Serial.print(F("Received packet of size "));
      Serial.println(packetSize);
      Serial.print(F("From "));
      IPAddress remoteIp = Udp.remoteIP();
      Serial.print(remoteIp);
      Serial.print(F(", port "));
      Serial.println(Udp.remotePort());
    }
    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    delay(100);
    return packetBuffer;
  }
  return "";
}

void setethernet() {
  Serial.println("in de set Ethernet");
  delay(500);
  Ethernet.begin(mac, ip, gateway, subnet);
  delay(2000);
  Serial.print(F("IP = "));
  Serial.println(Ethernet.localIP());
  setudp();
  checkping();
  delay(30);
  //sendUDPmessage(" ");
}

void setudp() {
  Udp.stop();
  if (DEBUG) {
    Serial.print(F("UDP stopped..."));
  }
  delay(100);
  Udp.begin(localPort);
  if (DEBUG) {
    Serial.print(F("Listening on port "));
    Serial.println(localPort);
  }
  delay(10);
}

bool checkping() {
  if (millis() - lastPing > 10000) {
    lastPing = millis();
    int pingResult = 0;
    pingResult = client.connect("192.168.15.4", 80);
    //Serial.print(F("result van pingResult: ")); Serial.println(pingResult);
    if (pingResult == 1) {
      if (DEBUG) {
        //Serial.println(F("ping ok - connected...."));
      }
      client.stop();
      return 1;
    } else {
      //try to ping again
      delay(100);
      pingResult = client.connect("192.168.15.4", 80);      
      if (pingResult == 1) {      
        client.stop();
        return 1;
      }  
      else {  
        if (DEBUG) {
        Serial.println(F("can not ping...."));
        }    
        return 0;
      }
    }
  }
  else {
    return 1;
  }
}

void valveBovenopen() {
  if (DEBUG)
  {
    Serial.println(F("valve boven moet nu open"));
  }
  digitalWrite(relayBoven, HIGH);
  sendUDPmessage("REPLY=valveBoven:open");
  delay(200);
}

void valveBovenclose() {
  if (DEBUG)
  {
    Serial.println(F("valve onder moet nu toe"));
  }
  digitalWrite(relayBoven, LOW);
  sendUDPmessage("REPLY=valveBoven:close");
  delay(200);
}

void valveOnderopen() {
  if (DEBUG)
  {
    Serial.println(F("valve moet nu open"));
  }
  digitalWrite(relayOnder, HIGH);
  sendUDPmessage("REPLY=valveOnder:open");
  delay(200);
}

void valveOnderclose() {
  if (DEBUG)
  {
    Serial.println(F("valve moet nu toe"));
  }
  digitalWrite(relayOnder, LOW);
  sendUDPmessage("REPLY=valveOnder:close");
  delay(200);
}
