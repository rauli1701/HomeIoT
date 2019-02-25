
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>

RF24 radio(7, 8); // CE, CSN
const byte address[6] = "var01";
char msg[1];
String theMessage = "";
int charpos = 0;
char charr[55];
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_HIGH);
  radio.startListening();
}

void loop()
{
  if (radio.available())
  {
    //read from the pipe
    radio.read(msg, 1);
    //store it on theChar
    char theChar = msg[0];
    //Serial.println("First if");
    if (msg[0] != '.')
    {

      charr[charpos] = theChar;
      ++charpos;
      //concat from char to string that the json can read it
      theMessage.concat(theChar);
      //Serial.println("Second if");
    }

    else

    {

      Serial.println(charpos);
      Serial.println(theMessage.length());

      theMessage = "";
      //Serial.println("else");
    }
  }
}