#include <Event.h>
#include <Timer.h>

Timer t;

//Setting up wireless for communication
#include <SPI.h>
#include <nRF24L01.h>
#include <printf.h>
#include <RF24.h>
#include <RF24_config.h>

RF24 radio(7,8); //CE, CSN

const byte address[6] = "var01";


//Setting up temperature sensors
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS_1 52
OneWire oneWire1(ONE_WIRE_BUS_1);
DallasTemperature sensors1(&oneWire1);

#define ONE_WIRE_BUS_2 53
OneWire oneWire2(ONE_WIRE_BUS_2);
DallasTemperature sensors2(&oneWire2);

#include <Adafruit_MAX31865.h>
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 max1 = Adafruit_MAX31865(34, 35, 32, 33);
Adafruit_MAX31865 max2 = Adafruit_MAX31865(31, 30, 29, 28);

#define RREF      430.0
#define RNOMINAL  100.0

#define WITH_LCD 1



DeviceAddress SensorSerial[11]= {

{0x28,0xFF,0xE7,0x24,0x81,0x17,0x05,0xC9},
{0x28,0xFF,0x3E,0xEE,0x80,0x17,0x05,0xED},
{0x28,0xFF,0xA8,0x9F,0x81,0x17,0x04,0x14},
{0x28,0xFF,0xF0,0xB4,0x81,0x17,0x04,0x5B},
{0x28,0xFF,0x97,0xE5,0x80,0x17,0x05,0xB9},
{0x28,0xFF,0x8E,0xEA,0x80,0x17,0x05,0xAC},
{0x28,0xFF,0x8C,0x0C,0x81,0x17,0x05,0xC4},
{0x28,0xFF,0x88,0x20,0x81,0x17,0x05,0xB9},
{0x28,0xFF,0x31,0x74,0xA3,0x16,0x05,0xCC},
{0x28,0xFF,0x99,0x22,0x81,0x17,0x05,0x0F}
};


//Setting up sensors

float oldtulipesa=0, oldsavukaasu=0, oldlVesi=0, oldpVesi=0, oldsahLVesi=0, oldsahPVesi=0, oldvaraus=0, oldkHuone=0;

float tulipesa, savukaasu, lVesi, pVesi, sahLVesi, sahPVesi, varaus, kHuone;

//Setting up rotary encoder, LCD and the rest
#include <ClickEncoder.h>
#include <TimerOne.h>

#ifdef WITH_LCD
#include <LiquidCrystal.h>

#define LCD_RS       22
#define LCD_EN       2
#define LCD_D4       27
#define LCD_D5       25
#define LCD_D6       24
#define LCD_D7       23

#define LCD_CHARS   16
#define LCD_LINES    2

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#endif

ClickEncoder *encoder;
int16_t last, value;

void timerIsr() {
  encoder->service();
}

#ifdef WITH_LCD
void displayAccelerationStatus() {
 // lcd.setCursor(0, 1); 
 // lcd.print("Acceleration ");
 // lcd.print(encoder->getAccelerationEnabled() ? "on " : "off");
}
#endif


//For the menu
String menuItems[] = {"Tulipesä", "Savukaasu", "L.Veden lämpö", "P.Veden Lämpö", "S.Vas L.Vesi", "S.Vas T.Vesi", "Varaus", "Kattilahuone", "Max ja Min"};
  int menu = 1;
  int frame = 1;
  int menuTick = 0;


//Temperature sensor functions
float getTemperatureSensors1(byte j)
{

sensors1.requestTemperaturesByAddress(SensorSerial[j]);

float tempC = sensors1.getTempC(SensorSerial[j]);

return tempC;

}


float getTemperatureSensors2(byte j)
{

sensors2.requestTemperaturesByAddress(SensorSerial[j]);

float tempC = sensors2.getTempC(SensorSerial[j]);

return tempC;

}





void setup() {
  Serial.begin(9600);
  encoder = new ClickEncoder(20, 21, 13, 4);
  sensors1.begin();
  sensors2.begin();

  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
  
#ifdef WITH_LCD
  lcd.begin(LCD_CHARS, LCD_LINES);
  //lcd.clear();
  //displayAccelerationStatus();
#endif
  lcd.print(menuItems[0]);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
 
  last = -1;
  //t.every(30000, sendReading,0);


  max1.begin(MAX31865_3WIRE);
  max2.begin(MAX31865_3WIRE);
}

void loop() {
  //t.update();
  value += encoder->getValue();

 
  tulipesa = max1.temperature(RNOMINAL, RREF);
  savukaasu = max2.temperature(RNOMINAL, RREF);
  lVesi = getTemperatureSensors1(0);
  pVesi = getTemperatureSensors1(1);
  sahLVesi = getTemperatureSensors2(3);
  sahPVesi = getTemperatureSensors2(4);
  varaus = getTemperatureSensors1(2);
  kHuone = getTemperatureSensors2(5);


  

  if (tulipesa != oldtulipesa || savukaasu != oldsavukaasu || lVesi != oldlVesi || pVesi != oldpVesi || sahLVesi != oldsahLVesi || sahPVesi != oldsahPVesi || varaus  != oldvaraus || kHuone != oldkHuone) {
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
  }
  

 
  if (value != last) {
    
    Serial.print("Menu is ");
    Serial.println(menu);
    Serial.print("Last is ");
    Serial.println(last);
    Serial.print("Value is ");
    Serial.println(value);
    
    if (value < last){
      menu--;
      last = value;
      menuTick = 0;
      if (menu <= 0) {
        menu = 9;
      }
      
    }
    
    else if (value > last) {
      menu++;
      last = value;
      menuTick = 0;
      if (menu >= 10) {
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

  
  switch (menu) {
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
    
      
  }

  if (frame != menu && menuTick == 0) {
    if (menu == 1) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[0]);
      lcd.setCursor(0,2);
      lcd.print(tulipesa);
      frame = menu;
      menuTick = 1;
    }
    else if (menu == 2){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[1]);
      lcd.setCursor(0,2);
      lcd.print(savukaasu);
      frame = menu;
      menuTick = 1;
    }
    else if (menu == 3) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[2]);
            lcd.setCursor(0,2);
      lcd.print(lVesi);
      frame = menu;
      menuTick = 1;
    }
        else if (menu == 4) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[3]);
            lcd.setCursor(0,2);
      lcd.print(pVesi);
      frame = menu;
      menuTick = 1;
    }
        else if (menu == 5) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[4]);
            lcd.setCursor(0,2);
      lcd.print(sahLVesi);
      frame = menu;
      menuTick = 1;
    }
        else if (menu == 6) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[5]);
            lcd.setCursor(0,2);
      lcd.print(sahPVesi);
      frame = menu;
      menuTick = 1;
    }
        else if (menu == 7) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[6]);
            lcd.setCursor(0,2);
      lcd.print(varaus);
      frame = menu;
      menuTick = 1;
    }
        else if (menu == 8) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[7]);
            lcd.setCursor(0,2);
      lcd.print(kHuone);
      frame = menu;
      menuTick = 1;
    }
        else if (menu == 9) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(menuItems[8]);
      frame = menu;
      menuTick = 1;
    }
  }
 
  ClickEncoder::Button b = encoder->getButton();
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



/*void sendReading() {
 sensor1 = getTemperature(7);
 sensor2 = getTemperature(5);

  String s =  "sensor1=" + String(sensor1) + ";sensor2="  + String(sensor2);
  int fuku = s.length()+1;
  char text[fuku];
  s.toCharArray(s.length()+1,s.length()+1);
  radio.write(&s,sizeof(s));
  //char s[] = "Yeeyee";
  radio.write(&s, sizeof(s));

}
*/
