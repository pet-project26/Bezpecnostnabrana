#include <WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>

WiFiServer server(80);

LiquidCrystal_I2C lcd(0x27, 16, 2);  // SDA_21 SCL_22

#define pinServo 4                   // Servo_4
Servo servomotor;

#define pinMFRC522rst 15           // RST_15
MFRC522DriverPinSimple ss_pin(5);  // SDA_5
MFRC522DriverSPI driver{ ss_pin };
MFRC522 mfrc522{ driver };

String header;
String stav = "zatvorena";
String uid = "";  // povoleny tag - > b6475e5e

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  mfrc522.PCD_Init();
  pinMode(pinMFRC522rst, OUTPUT);
  digitalWrite(pinMFRC522rst, HIGH);

  pinMode(2, OUTPUT);
  servomotor.setPeriodHertz(50);
  servomotor.attach(pinServo, 500, 2400);
  servomotor.write(0);
  delay(1500);
  servomotor.detach();

  WiFi.begin("peterjen", "peterjen");
  lcd.print("Pripajam");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }

  Serial.println("WiFi pripojena!");
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.print(">");
  lcd.print(WiFi.localIP());
  lcd.setCursor(0, 1);
  lcd.print("brana:zatvorena");

  server.begin();
  Serial.println("INIT OK!");
}

void loop() {
  
  if (mfrc522.PICC_IsNewCardPresent())  // ...ak sa nacitala karta...
  {
    if (mfrc522.PICC_ReadCardSerial()) {
      uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10)
          uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }
    }

    Serial.println("ID karty:" + String(uid));
    if (uid == "b6475e5e") {
      if (stav == "zatvorena")
        otvor();
      else  //if(stav == "otvorena")
        zatvor();
    }
  }

  WiFiClient client = server.available();  // aktualizovanie prijatych dat

  if (client)  // ak sa pripojil klient...
  {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("Novy klient");
    String currentLine = "";  // premenna pre prijate data

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available())  // test ci su prijate dake data
      {
        char c = client.read();  // nacitanie bajtu z buffera
        Serial.write(c);
        header += c;

        if (c == '\n')  // po ENTER je request
        {
          if (currentLine.length() == 0)  // ak sa nacital prazdny riadok, odosli potvrdenie o spojeni
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println("");



            if (header.indexOf("GET /otvoreny") >= 0) {
              otvor();
            } else if (header.indexOf("GET /zatvoreny") >= 0) {
              zatvor();
            }

            client.println("<!DOCTYPE html><html><head>");
            client.println("<style>");  // integracia CSS
            client.println(".button {");
            client.println("padding: 24px 48px;");
            client.println("font-size: 18px;");
            client.println("background-color: #007bff;");
            client.println("color: white;");
            client.println("border: none;");
            client.println("cursor: pointer;");
            client.println("}");
            client.println("</style>");
            client.println("</head><body>");
            client.println("<div align='center'>");

            client.println("<h1>Otvaranie brany</h1><br>");

            client.println("<p>STAV BRANY: <b>" + stav + "<b></p>");
            if (stav == "zatvorena")
              client.println("<p><a href=\"/otvoreny\"><button class=\"button\">OTVORIT</button></a></p>");
            else
              client.println("<p><a href=\"/zatvoreny\"><button class=\"button button2\">ZATVORIT</button></a></p>");

            client.println("</div>");
            client.println("</body></html>");
            client.println("");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;  // add it to the end of the currentLine
        }
      }
    }

    header = "";
    client.stop();
    Serial.println("Klient je odpojeny.");
    Serial.println("");
  }
}

void otvor() {
  Serial.println("STATUS: OTVORENY");
  stav = "otvorena";
  Serial.println(stav);
  lcd.setCursor(0, 1);
  lcd.print("brana:otvorena ");
  digitalWrite(2, HIGH);
  servomotor.attach(pinServo, 500, 2400);
  servomotor.write(180);
  delay(1500);
  servomotor.detach();
}

void zatvor() {
  Serial.println("STATUS: ZATVORENY");
  stav = "zatvorena";
  Serial.println(stav);
  lcd.setCursor(0, 1);
  lcd.print("brana:zatvorena");
  digitalWrite(2, LOW);
  servomotor.attach(pinServo, 500, 2400);
  servomotor.write(0);
  delay(1500);
  servomotor.detach();
}