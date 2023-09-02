#include <util/crc16.h>;

void setup()
{
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
}

// uint8_t checkcrc()
// {
//   uint8_t crc = 0, i;

//   for (i = 0; i < sizeof serno / sizeof serno[0]; i++)
//     crc = _crc_ibutton_update(crc, serno[i]);

//   return crc; // must be 0
// }

// the loop function runs over and over again forever
void loop()
{

  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(500);                      // wait for a second
  Serial.write(analogRead(A0));
  digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  delay(500);                     // wait for a second
}