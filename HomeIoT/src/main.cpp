#include <Event.h>
#include <Timer.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#define MQTT_MAX_PACKET_SIZE 256;

Timer t;

//Setting Ethernet and MQTT

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 105);
IPAddress server(192, 168, 1, 106);

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;
//Setting up temperature sensors
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS_1 44
OneWire oneWire1(ONE_WIRE_BUS_1);
DallasTemperature sensors1(&oneWire1);

#define ONE_WIRE_BUS_2 45
OneWire oneWire2(ONE_WIRE_BUS_2);
DallasTemperature sensors2(&oneWire2);

#include <Adafruit_MAX31865.h>
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 max1 = Adafruit_MAX31865(34, 35, 32, 33);
//Adafruit_MAX31865 max2 = Adafruit_MAX31865(31, 30, 29, 28);
Adafruit_MAX31865 max2 = Adafruit_MAX31865(30, 31, 28, 29);

#define RREF 430.0
#define RNOMINAL 100.0

#define WITH_LCD 1

DeviceAddress SensorSerial[11] = {

    {0x28, 0xFF, 0xE7, 0x24, 0x81, 0x17, 0x05, 0xC9},
    {0x28, 0xFF, 0x3E, 0xEE, 0x80, 0x17, 0x05, 0xED},
    {0x28, 0xFF, 0xA8, 0x9F, 0x81, 0x17, 0x04, 0x14},
    {0x28, 0xFF, 0xF0, 0xB4, 0x81, 0x17, 0x04, 0x5B},
    {0x28, 0xFF, 0x97, 0xE5, 0x80, 0x17, 0x05, 0xB9},
    {0x28, 0xFF, 0x8E, 0xEA, 0x80, 0x17, 0x05, 0xAC},
    {0x28, 0xFF, 0x8C, 0x0C, 0x81, 0x17, 0x05, 0xC4},
    {0x28, 0xFF, 0x88, 0x20, 0x81, 0x17, 0x05, 0xB9},
    {0x28, 0xFF, 0x31, 0x74, 0xA3, 0x16, 0x05, 0xCC},
    {0x28, 0xFF, 0x99, 0x22, 0x81, 0x17, 0x05, 0x0F}};

//Setting up sensors

float oldtulipesa = 0, oldsavukaasu = 0, oldlVesi = 0, oldpVesi = 0, oldsahLVesi = 0, oldsahPVesi = 0, oldvaraus = 0, oldkHuone = 0;

float tulipesa, savukaasu, lVesi, pVesi, sahLVesi, sahPVesi, varaus, kHuone;

//Alerts

const int aKayntitietoPin = 38;
const int aYlilampoPin = 39;
const int aYlipitSyottoPin = 40;
const int aLamporeleLauennutPin = 36;
const int aKayntisallittu = 37;

int generalAlert = 0;
int clearAll = 0;
String alertStatus = "";
/*
bool kayntitieto, ylilampo, ylipitkasyotto, lamporeleLauennut, kayntisallittu;
*/

//Setting up rotary encoder, LCD and the rest
#include <ClickEncoder.h>
#include <TimerOne.h>

#ifdef WITH_LCD
#include <LiquidCrystal.h>

#define LCD_RS 22
#define LCD_EN 6
#define LCD_D4 27
#define LCD_D5 25
#define LCD_D6 24
#define LCD_D7 23

#define LCD_CHARS 16
#define LCD_LINES 2

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#endif

byte customChar[8] = {
	0b01000,
	0b10000,
	0b01010,
	0b00110,
	0b01110,
	0b01111,
	0b11011,
	0b01011
};

int retries = 0;
long mytime;
long yourtime = 0;
ClickEncoder *encoder;
int16_t last, value;

void timerIsr()
{
  encoder->service();
}

#ifdef WITH_LCD
void displayAccelerationStatus()
{
  // lcd.setCursor(0, 1);
  // lcd.print("Acceleration ");
  // lcd.print(encoder->getAccelerationEnabled() ? "on " : "off");
}
#endif

//For the menu
String menuItems[] = {"Tulipesa", "Savukaasu", "L.Veden lampo", "P.Veden Lampo", "S.Vas L.Vesi", "S.Vas T.Vesi", "Varaus", "Kattilahuone", "Max ja Min", "Poltin"};
int menu = 1;
int frame = 1;
int menuTick = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

boolean reconnect() {
  if (client.connect("arduinoClient")) {
    // Once connected, publish an announcement...
    Serial.println("Connected again");
    retries = 0;
    
  }
  else {
    retries++;
    Serial.println("Attempt number " + retries); 

  }
  return client.connected();
}

//Temperature sensor functions
float getTemperatureSensors1(byte j)
{

  sensors1.requestTemperaturesByAddress(SensorSerial[j]);

  float tempC = sensors1.getTempC(SensorSerial[j]);

  return tempC;
}

float getTemperatureSensors2(byte k)
{

  sensors2.requestTemperaturesByAddress(SensorSerial[k]);

  float tempC = sensors2.getTempC(SensorSerial[k]);

  return tempC;
}

void sendMeasures() {

  int fok = client.state();

  switch (fok) {
    case -4:
      Serial.println("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time");
      break;
    
    case -3:
      Serial.println("MQTT_CONNECTION_LOST - the network connection was broken");
      break;
    
    case -2:
      Serial.println("MQTT_CONNECT_FAILED - the network connection failed");
      break;
    
    case -1:
      Serial.println("MQTT_DISCONNECTED - the client is disconnected cleanly");
      break;
    
    case 0: 
      Serial.println("MQTT_CONNECTED - the client is connected");
      break;
    
    case 1: 
      Serial.println("MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT");
      break;
    
    case 2: 
      Serial.println("MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier");
      break;
    
    case 3: 
      Serial.println("MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection");
      break;
    
    case 4: 
      Serial.println("MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected");
      break;
    
    case 5: 
      Serial.println("MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect");
      break;
  
  }



savukaasu = max1.temperature(RNOMINAL, RREF);
tulipesa = max2.temperature(RNOMINAL, RREF);
lVesi = getTemperatureSensors1(0);
pVesi = getTemperatureSensors1(1);
sahLVesi = getTemperatureSensors2(3);
sahPVesi = getTemperatureSensors2(4);
varaus = getTemperatureSensors1(2);
kHuone = getTemperatureSensors2(5);
String transfer = "{\"Savukaasu\":" + String(savukaasu) + ",\"Tulipesa\":" + String(tulipesa) + ",\"Lvesi\":" + String(lVesi) + ",\"Pvesi\":" + String(pVesi) + ",\"SahlVesi\":" + String(sahLVesi) + ",\"SahPVesi\":" + String(sahPVesi) + ",\"Varaus\":" + String(varaus) + ",\"Khuone\":" + String(kHuone) + ",\"kayntitieto\":" + String(digitalRead(aKayntitietoPin)) + ",\"ylilampo\":" + String(digitalRead(aYlilampoPin)) + ",\"ylipitkasyotto\":" + String(digitalRead(aYlipitSyottoPin)) + ",\"Lamporelel\":" + String(digitalRead(aLamporeleLauennutPin)) + ",\"kayntisallittu\":" + String(digitalRead(aKayntisallittu)) + "}";
delay(1000);
Serial.println(transfer);
int transferSize = transfer.length();
Serial.println(transferSize);
delay(1000);

Serial.println("I am here, before payload");
char payload[] = "";
transfer.getBytes(payload, transferSize);
/*
client.publish("kattilat", payload);

Serial.println("I am here, after payload, before transfer");

 */
/*if (client.connect("arduinoClient", "varasto", "Varasto1")) {
    client.publish("kattilat", payload);
    
  }
else {
  Serial.println("No connection");
}
 */
/*

if (!client.connected()) {
  Serial.println("Shit1");
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("Shit0");
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
        Serial.println("Shit");
        client.publish("kattilat", payload);
        Serial.println("I have published mah shit after reconnect");
        
      }
    }
  } else {
    // Client connected
    client.publish("kattilat", payload);
    
    Serial.println("I have published mah shit");
    
  }

Serial.println("I am here, after transfer");
 */
Serial.println("Whats up dawg?");

}

void setup()
{
  lcd.createChar(0, customChar);
  //Encoder and serial
  Serial.begin(9600);
  encoder = new ClickEncoder(20, 21, 9, 4);
  //Sensors
  sensors1.begin();
  sensors2.begin();
  max1.begin(MAX31865_3WIRE);
  max2.begin(MAX31865_3WIRE);
  //Video killed the radio star
  client.setServer(server, 1883);
  client.setCallback(callback);
  //LCD
#ifdef WITH_LCD
  lcd.begin(LCD_CHARS, LCD_LINES);
#endif
  lcd.print(menuItems[0]);
  //Timers
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
  //t.every(15000, sendMeasures);
  last = -1;
 
  //Alerts
  pinMode(aKayntisallittu, INPUT);
  pinMode(aKayntitietoPin, INPUT);
  pinMode(aLamporeleLauennutPin, INPUT);
  pinMode(aYlilampoPin, INPUT);
  pinMode(aYlipitSyottoPin, INPUT);
  
  Ethernet.begin(mac, ip);

  lastReconnectAttempt = 0;

  mytime = millis();
}

void loop()
{
  sendMeasures();
  client.loop();  
  alertStatus = "";
  //Updating timers and encoder
  
  value += encoder->getValue();
  //Getting values from sensors
  //10,9,8,7 Muistilappu
  savukaasu = max1.temperature(RNOMINAL, RREF);
  tulipesa = max2.temperature(RNOMINAL, RREF);
  lVesi = getTemperatureSensors1(0);
  pVesi = getTemperatureSensors1(1);
  sahLVesi = getTemperatureSensors2(3);
  sahPVesi = getTemperatureSensors2(4);
  varaus = getTemperatureSensors1(2);
  kHuone = getTemperatureSensors2(5);
  Serial.println("I am here, i have received info from sensors");
  /*
bool kayntitieto, ylilampo, ylipitkasyotto, lamporeleLauennut, kayntisallittu;
*/

  

    if ((digitalRead(aYlilampoPin) == HIGH && generalAlert == 0 )|| (digitalRead(aYlipitSyottoPin) == HIGH && generalAlert == 0)|| (digitalRead(aLamporeleLauennutPin) == HIGH && generalAlert == 0))
    {
      if (digitalRead(aYlilampoPin) == HIGH)
      {
        alertStatus += " Ylilampo";
        clearAll = 1;
        Serial.println("I am here, i hace found pin for overheating");
      }
      if (digitalRead(aYlipitSyottoPin) == HIGH)
      {
        alertStatus += " Ylipitkasyotto";

        clearAll = 1;
        Serial.println("I am here, I have found too long feed");
      }
      if (digitalRead(aLamporeleLauennutPin) == HIGH)
      {
        alertStatus += " Lamporele lauennut";
        clearAll = 1;
        Serial.println("I am here, i have found lamporelet ja perkele se paukkuu");
      }
      int alertL = alertStatus.length();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(alertStatus);
      if (alertL > 16)
      {

        for (int positionCounter = 0; positionCounter < alertL; positionCounter++)
        {

          lcd.scrollDisplayLeft();

          delay(200);
        }
      }
      Serial.println(generalAlert);
      Serial.println(clearAll);
      Serial.println("I have printed alerts");
       ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    switch (b) {
      
      case ClickEncoder::Pressed:
          Serial.print("Pressed");
          generalAlert = 1;
          
          break;

      case ClickEncoder::Held:
          Serial.print("Held");
          break;

      case ClickEncoder::Released:
          Serial.print("Released");
          break;

      case ClickEncoder::Clicked:
          Serial.print("Clicked");
          generalAlert = 1;
          
          break;
     
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
  
          Serial.print("  Acceleration is ");
          Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
          break;
    }
  }    
    }

    else
    {

          Serial.println("I am here, in the main menu");
    if (digitalRead(aKayntitietoPin == HIGH))
    {
      
      lcd.setCursor(15, 0);
      lcd.write((uint8_t)0);
    }

    else {
      lcd.setCursor(15,0);
      lcd.write(" ");
    }
      



      //Updating menus
      if (tulipesa != oldtulipesa || savukaasu != oldsavukaasu || lVesi != oldlVesi || pVesi != oldpVesi || sahLVesi != oldsahLVesi || sahPVesi != oldsahPVesi || varaus != oldvaraus || kHuone != oldkHuone)
      {
        frame = 0;
        menuTick = 0;
        oldkHuone = kHuone;
        oldlVesi = lVesi;
        oldpVesi = pVesi;
        oldsahLVesi = sahLVesi;
        oldsahPVesi = sahPVesi;
        oldsavukaasu = savukaasu;
        oldtulipesa = tulipesa;
        oldvaraus = varaus;
        Serial.println("I am here, i have updated menus");
      }

      //Debugging decoder controls
      if (value != last)
      {

        Serial.print("Menu is ");
        Serial.println(menu);
        Serial.print("Last is ");
        Serial.println(last);
        Serial.print("Value is ");
        Serial.println(value);

        if (value < last)
        {
          menu--;
          last = value;
          menuTick = 0;
          if (menu <= 0)
          {
            menu = 11;
          }
        }

        else if (value > last)
        {
          menu++;
          last = value;
          menuTick = 0;
          if (menu >= 12)
          {
            menu = 1;
          }
        }

        Serial.print("Encoder Value: ");
        Serial.println(value);
#ifdef WITH_LCD
        //lcd.setCursor(0, 2);
        //lcd.print("         ");
        //lcd.setCursor(0, 0);
        //lcd.print(value);
#endif
      }

      //Menus

      switch (menu)
      {
      case 1:
        frame = 0;
        break;
      case 2:
        frame = 0;
        break;
      case 3:
        frame = 0;
        break;
      case 4:
        frame = 0;
        break;
      case 5:
        frame = 0;
        break;
      case 6:
        frame = 0;
        break;
      case 7:
        frame = 0;
        break;
      case 8:
        frame = 0;
        break;
      case 9:
        frame = 0;
        break;
      case 10:
        frame = 0;
        break;
      case 11:
        frame = 0;
        break;
      }

      if (frame != menu && menuTick == 0)
      {
        if (menu == 1)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[0]);
          lcd.setCursor(0, 2);
          lcd.print(tulipesa);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 2)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[1]);
          lcd.setCursor(0, 2);
          lcd.print(savukaasu);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 3)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[2]);
          lcd.setCursor(0, 2);
          lcd.print(lVesi);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 4)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[3]);
          lcd.setCursor(0, 2);
          lcd.print(pVesi);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 5)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[4]);
          lcd.setCursor(0, 2);
          lcd.print(sahLVesi);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 6)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[5]);
          lcd.setCursor(0, 2);
          lcd.print(sahPVesi);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 7)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[6]);
          lcd.setCursor(0, 2);
          lcd.print(varaus);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 8)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[7]);
          lcd.setCursor(0, 2);
          lcd.print(kHuone);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 9)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(menuItems[8]);
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 10)
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(menuItems[9]);
          lcd.setCursor(0,2);
          if (digitalRead(aKayntisallittu) == HIGH) {
            lcd.print("Sallittu");
          }
          else {
            lcd.print("Estolla");
          }
          frame = menu;
          menuTick = 1;
        }
        else if (menu == 11)
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Hälyt: ");
          if (clearAll > 0) {
            lcd.print("Ei kuitattu");
          }
          else {
            lcd.print("Kuitattu");
          }
          lcd.setCursor(0,2);
          ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    switch (b) {
      
      case ClickEncoder::Pressed:
          Serial.print("Pressed");
          generalAlert = 0;
          clearAll = 0;
          break;

      case ClickEncoder::Held:
          Serial.print("Held");
          break;

      case ClickEncoder::Released:
          Serial.print("Released");
          break;

      case ClickEncoder::Clicked:
          Serial.print("Clicked");
          generalAlert = 0;
          clearAll = 0;
          break;
     
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
  
          Serial.print("  Acceleration is ");
          Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
          break;
    }
  }    


          frame = menu;
          menuTick = 1;
        }
      }
      //Decoder buttons
     // ClickEncoder::Button b = encoder->getButton();
      /* if (b != ClickEncoder::Open) {
    Serial.print("Button: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::DoubleClicked:
          Serial.println("ClickEncoder::DoubleClicked");
          encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
          Serial.print("  Acceleration is ");
          Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
#ifdef WITH_LCD
          displayAccelerationStatus();
#endif
        break;
    }
  } */
    }
    //}
  /*
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("Poltin");
    lcd.setCursor(0, 1);
    lcd.write("estolla");
  }
   */
  t.update();
}
