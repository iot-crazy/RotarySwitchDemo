#include <Arduino.h>
#include <U8g2lib.h>
#include <U8glib.h>

#define CONFIG_ESP_INT_WDT_TIMEOUT_MS 500;

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

/* OLED screen setup */
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/16, /* clock=*/15, /* data=*/4); // ESP32 Thing, HW I2C with pin remapping
int screenWidth = 128;
int screenHeight = 64;

/* Pin assignments */
int pinA = 17;      // Connected to CLK on KY-040
int pinB = 22;      // Connected to DT on KY-040
int pinSwitch = 13; // The push switch

int pinALast;
int switchIsPressedStartTime = 0;
boolean switchIsPressed = false;
int longPressTime = 3000;
boolean isReset = false;
int digitPos = 3;
int charWidth = 0;
int allDigitsWidth = 0;
int charHeight = 0;
bool digitsChanged = true;
int digits[3] = {0, 0, 0};
int x = 0;
int y = 0;
int rotaryAction = 0;

void u8g2_prepare(void)
{
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void updateDisplay()
{
  if (digitsChanged)
  {
    u8g2.clearBuffer();
    char msg[3];
    for (int i = 0; i < 3; i++)
    {
      msg[i] = digits[i] + '0';
    }
    u8g2.drawStr(x, y, msg);

    if (digitPos < 3)
    {
      u8g2.drawDisc(x + (charWidth * (digitPos + 1) - (charWidth / 2)), y + charHeight - 2, 3);
    }

    u8g2.sendBuffer();
    digitsChanged = false;
  }
}

int getCounter()
{
  int counter = 0;
  counter += digits[2];
  counter += digits[1] * 10;
  counter += digits[0] * 100;
  return counter;
}

void ChangeCounter(int amount)
{
  int counter = getCounter() + amount;
  if (counter > 999)
  {
    counter = 0;
  }
  else if (counter < 0)
  {
    counter = 999;
  }

  for (int t = 2; t >= 0; t--)
  {
    digits[t] = counter % 10;
    counter = counter / 10;
  }

  digitsChanged = true;
}

void ChangeCounterAtPosition(int *position, int amount)
{
  int digit = digits[*position];

  digit += amount;
  if (digit > 9)
  {
    digit = 0;
  }
  else if (digit < 0)
  {
    digit = 9;
  }
  digits[*position] = digit;
  digitsChanged = true;
}

void doRotary()
{
  int aVal = digitalRead(pinA);

  if (aVal != pinALast && switchIsPressed == false)
  { // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(pinB) != aVal)
    { // Means pin A Changed first - We're Rotating Clockwise
      rotaryAction = 1;
    }
    else
    { // Otherwise B changed first and we're moving CCW
      rotaryAction = 2;
    }
  }
  pinALast = aVal;
}

void OnSwitchChanged()
{
  int switchState = digitalRead(pinSwitch);
  if (switchState == HIGH && switchIsPressed == true)
  {
    switchIsPressed = false;

    if (isReset == false)
    {
      digitPos++;
      if (digitPos > 3)
      {
        digitPos = 0;
      }
    }
    isReset = false;
    digitsChanged = true;
  }
  else if (switchIsPressed == false)
  {
    switchIsPressedStartTime = millis();
    switchIsPressed = true;
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting...");
  u8g2.begin();

  u8g2_prepare();
  u8g2.clearBuffer();
  u8g2.drawStr(40, 32, "Starting...");
  u8g2.drawCircle(64, 42, 4);
  u8g2.sendBuffer();

  u8g2.setFont(u8g2_font_inb49_mn);
  u8g2.setFontRefHeightExtendedText();
  charWidth = u8g2.getStrWidth("0");
  allDigitsWidth = charWidth * 3;
  charHeight = u8g2.getMaxCharHeight();
  x = (screenWidth - allDigitsWidth) / 2;
  y = (screenHeight - charHeight) / 2;

  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);
  pinMode(pinSwitch, INPUT);

  pinALast = digitalRead(pinA);
  delay(300);

  attachInterrupt(digitalPinToInterrupt(pinA), doRotary, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinSwitch), OnSwitchChanged, CHANGE);

  u8g2.clearBuffer();
  u8g2.sendBuffer();
}

void loop()
{

  if (rotaryAction == 1)
  {
    if (digitPos == 3)
    {
      ChangeCounter(1);
    }
    else
    {
      ChangeCounterAtPosition(&digitPos, 1);
    }
    rotaryAction = 0;
  }
  else if (rotaryAction == 2)
  {
    if (digitPos == 3)
    {
      ChangeCounter(-1);
    }
    else
    {
      ChangeCounterAtPosition(&digitPos, -1);
    }
    rotaryAction = 0;
  }

  if (isReset == false && switchIsPressed && (millis() - switchIsPressedStartTime) >= longPressTime)
  {
    digits[0] = 0;
    digits[1] = 0;
    digits[2] = 0;
    isReset = true;
    digitsChanged = true;
  }

  updateDisplay();
  //delay(5);
}