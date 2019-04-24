#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>

#define TIME_CHECK_INTERVAL 10000
#define LED_PIN 6
#define LED_COUNT 120
#define LED_BRIGHTNESS 128
#define BUTTON_PIN 8
#define BDAY_MONTH 4
#define BDAY_DAY 21

// CONTROLLERS

enum ControllerPattern { NONE, SINGLE_COLOR, RAINBOW };
enum PatternDirection { FORWARD, REVERSE };

class Controller
{
    public:
 
    // Member Variables:  
    ControllerPattern  ActivePattern = NONE;  // which pattern is running
    PatternDirection Direction = FORWARD;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long LastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    uint16_t Seed;
    
    Adafruit_NeoPixel &NeoPixel;

    const uint8_t StartPixel;
    const uint8_t EndPixel;
    
    void (*OnComplete)();  // Callback on completion of pattern

    Controller(Adafruit_NeoPixel &neoPixel, const uint8_t startPixel, const uint8_t endPixel, void (*callback)()) : NeoPixel(neoPixel), StartPixel(startPixel), EndPixel(endPixel)
    {
      OnComplete = callback;
    }

    void Update()
    {
      switch (ActivePattern)
      {
        case NONE:
        case SINGLE_COLOR:
          return;

        default:
          break;
      }
      
      unsigned long now = millis();
      
      if ((now - LastUpdate) < Interval) {
        return;
      }

      LastUpdate = now;

      switch (ActivePattern)
      {
        case RAINBOW:
          RainbowUpdate();
          return;

        default:
          return;
      }
    }

    void Increment()
    {
      if (Direction == FORWARD)
      {
        Index++;

        if (TotalSteps > 0 && Index >= TotalSteps)
        {
          Index = 0;

          if (OnComplete != NULL)
          {
            OnComplete();
          }
        }
      }
      else
      {
        --Index;
        
        if (TotalSteps > 0 && Index <= 0)
        {
          Index = TotalSteps - 1;

          if (OnComplete != NULL)
          {
            OnComplete();
          }
        }
      }
    }

    void Reverse()
    {
      if (Direction == FORWARD)
      {
        Direction = REVERSE;
        Index = TotalSteps - 1;
      }
      else
      {
        Direction = FORWARD;
        Index = 0;
      }
    }

    void SetSingleColor(uint32_t color)
    {
      ActivePattern = SINGLE_COLOR;
      Interval = 0;
      TotalSteps = 0;
      Index = 0;
      Direction = FORWARD;
      Seed = 0;

      Color1 = color;
      Color2 = 0;

      ColorSet(color);
    }

    void SetRainbow(uint8_t interval = 40, PatternDirection dir = FORWARD)
    {
      ActivePattern = RAINBOW;
      Interval = interval;
      TotalSteps = 255;
      Index = 0;
      Direction = dir;
      Seed = random(0, 255);

      Color1 = 0;
      Color2 = 0;
    }

    void RainbowUpdate()
    {
      uint16_t hue = ((Seed + Index) % 255) * 65536 / 255;
      
      ColorSet(NeoPixel.ColorHSV(hue));
      
      Increment();
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
      for (uint8_t i = StartPixel; i <= EndPixel; i++)
      {
        NeoPixel.setPixelColor(i, color);
      }
      
      NeoPixel.show();
    }

    void Clear()
    {
      if (ActivePattern == NONE)
      {
        return;
      }
      
      ActivePattern = NONE;
      Interval = 0;
      TotalSteps = 0;
      Index = 0;
      Direction = FORWARD;
      Seed = 0;

      Color1 = 0;
      Color2 = 0;
      
      ColorSet(NeoPixel.Color(0, 0, 0));
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.

    uint32_t Wheel(uint8_t  WheelPos)
    {
        WheelPos = 255 - WheelPos;
        
        if (WheelPos < 85)
        {
            return NeoPixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if (WheelPos < 170)
        {
            WheelPos -= 85;
            return NeoPixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return NeoPixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

// INIT

RTC_DS3231 rtc;

bool ButtonPressed = false;
unsigned long TimeSinceButtonPress = 0;
unsigned long ButtonPressLastUpdate = 0;

unsigned long LastTimeCheck = 32768;
uint8_t LastHr = 255;
uint8_t LastMin = 255;
uint8_t TimeHrOffset = 0;

enum SpecialPattern { SP_NONE, SP_HAPPY_BIRTHDAY };

SpecialPattern CurrentSpecialPattern = SP_NONE;

unsigned long SpecialPatternLastUpdate = 0;
uint8_t SpecialPatternIndex = 0;

// INIT NEOPIXEL
Adafruit_NeoPixel WC_Strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

Controller C_TWENTY(WC_Strip, 0, 5, NULL);
Controller C_IS(WC_Strip, 7, 8, NULL);
Controller C_IT(WC_Strip, 10, 11, NULL);
Controller C_QUARTER(WC_Strip, 12, 18, NULL);
Controller C_HALF(WC_Strip, 20, 23, NULL);
Controller C_HAPPY(WC_Strip, 24, 28, NULL);
Controller C_TEN(WC_Strip, 29, 31, NULL);
Controller C_FIVE(WC_Strip, 32, 35, NULL);
Controller C_MINUTES(WC_Strip, 36, 42, NULL);
Controller C_BIRTH(WC_Strip, 43, 47, NULL);
Controller C_DAY(WC_Strip, 48, 50, NULL);
Controller C_HR_ONE(WC_Strip, 51, 53, NULL);
Controller C_TO(WC_Strip, 55, 56, NULL);
Controller C_PAST(WC_Strip, 56, 59, NULL);
Controller C_HR_TWO(WC_Strip, 60, 62, NULL);
Controller C_HR_FIVE(WC_Strip, 63, 66, NULL);
Controller C_ALICE(WC_Strip, 67, 71, NULL);
Controller C_HR_TWELVE(WC_Strip, 72, 77, NULL);
Controller C_HR_ELEVEN(WC_Strip, 78, 83, NULL);
Controller C_HR_THREE(WC_Strip, 84, 88, NULL);
Controller C_HR_FOUR(WC_Strip, 89, 92, NULL);
Controller C_HR_SIX(WC_Strip, 93, 95, NULL);
Controller C_HR_TEN(WC_Strip, 96 ,98, NULL);
Controller C_HR_NINE(WC_Strip, 99, 102, NULL);
Controller C_HR_SEVEN(WC_Strip, 103, 107, NULL);
Controller C_HR_EIGHT(WC_Strip, 108, 112, NULL);
Controller C_OCLOCK(WC_Strip, 114, 119, NULL);

// BEGIN EXECUTION

void setup()
{
  Serial.begin(19200);

  Serial.println("Starting...");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  WC_Strip.begin();
  WC_Strip.setBrightness(LED_BRIGHTNESS);
}

void loop()
{
  checkButton();
  checkTime();
  updateSpecialPattern();
  updateControllers();
}

// BUTTON

void checkButton()
{
  // BUTTON IS NOT PRESSED
  
  if (digitalRead(BUTTON_PIN) != LOW)
  {
    if (ButtonPressed == true && (millis() - TimeSinceButtonPress) > 200)
    {
      Serial.println("Button UP");
      
      ButtonPressed = false;
    }
    
    return;
  }

  // BUTTON IS PRESSED
  
  unsigned long now = millis();

  if (ButtonPressed == false)
  {
    Serial.println("Button DOWN");
    
    ButtonPressed = true;
    TimeSinceButtonPress = now;
  }
  
  if ((now - TimeSinceButtonPress) < 3000)
  {
    return;
  }

  // BUTTON PRESSED FOR AT LEAST 3 SECONDS

  if ((now - ButtonPressLastUpdate) < 1000)
  {
    return;
  }

  ButtonPressLastUpdate = now;

  // 1 SECOND UPDATE INTERVAL

  TimeHrOffset++;

  if (TimeHrOffset >= 12)
  {
    TimeHrOffset = 0;
  }

  Serial.print("Updating time hour offset to ");
  Serial.print(TimeHrOffset, DEC);
  Serial.println();
  
  forceTimeUpdate();
}

// TIME

DateTime getTimeWithOffset()
{
  DateTime dt = rtc.now();

  if (TimeHrOffset != 0)
  {
    TimeSpan ts = TimeSpan((int32_t)TimeHrOffset * (int32_t)3600);

    dt = dt + ts;
  }

  return dt;
}

void checkTime()
{
  if (CurrentSpecialPattern != SP_NONE)
  {
    return;
  }
  
  unsigned long now = millis();

  // only do time check every x seconds
  if ((now - LastTimeCheck) < TIME_CHECK_INTERVAL) {
    return;
  }

  Serial.println("Checking the time...");

  LastTimeCheck = now;

  DateTime dt = getTimeWithOffset();

  uint8_t currentHr = dt.hour();
  uint8_t currentMin = dt.minute();
  
  Serial.print(currentHr, DEC);
  Serial.print(':');
  Serial.print(currentMin, DEC);
  Serial.println();

  // get minute interval
  uint8_t currentMinInterval = currentMin / 5;
  uint8_t lastMinInterval = LastMin / 5;

  // check if its time to update the leds (every 5 minutes)
  if (currentHr == LastHr && currentMinInterval == lastMinInterval)
  {
    return;
  }

  uint8_t currentMonth = dt.month();
  uint8_t currentDay = dt.day();

  if (currentMonth == BDAY_MONTH && currentDay == BDAY_DAY)
  {
    setSpecialPattern(SP_HAPPY_BIRTHDAY);
    return; 
  }
  
  updateTime(currentHr, currentMin);
}

void forceTimeUpdate()
{
  DateTime dt = getTimeWithOffset();

  uint8_t currentHr = dt.hour();
  uint8_t currentMin = dt.minute();
    
  updateTime(currentHr, currentMin);
}

void updateTime(uint8_t currentHr, uint8_t currentMin) {
  Serial.print("Updating the time to ");
  Serial.print(currentHr, DEC);
  Serial.print(':');
  Serial.print(currentMin, DEC);
  Serial.println();
  
  LastHr = currentHr;
  LastMin = currentMin;
  
  uint8_t currentMinInterval = currentMin / 5;

  // CLEAR ALL CONTROLLERS
  clearControllers();

  // IT IS
  C_IT.SetRainbow();
  C_IS.SetRainbow();

  switch (currentMinInterval) {
    // FIVE MINUTES PAST
    case 1:
      C_FIVE.SetRainbow();
      C_MINUTES.SetRainbow();
      C_PAST.SetRainbow();
      break;

    // TEN MINUTES PAST
    case 2:
      C_TEN.SetRainbow();
      C_MINUTES.SetRainbow();
      C_PAST.SetRainbow();
      break;

    // QUARTER PAST
    case 3:
      C_QUARTER.SetRainbow();
      C_PAST.SetRainbow();
      break;

    // TWENTY MINUTES PAST
    case 4:
      C_TWENTY.SetRainbow();
      C_MINUTES.SetRainbow();
      C_PAST.SetRainbow();
      break;

    // TWENTY FIVE MINUTES PAST
    case 5:
      C_TWENTY.SetRainbow();
      C_FIVE.SetRainbow();
      C_MINUTES.SetRainbow();
      C_PAST.SetRainbow();
      break;

    // HALF PAST
    case 6:
      C_HALF.SetRainbow();
      C_PAST.SetRainbow();
      break;

    // TWENTY FIVE MINUTES TO
    case 7:
      currentHr++;
      C_TWENTY.SetRainbow();
      C_FIVE.SetRainbow();
      C_MINUTES.SetRainbow();
      C_TO.SetRainbow();
      break;

    // TWENTY MINUTES TO
    case 8:
      currentHr++;
      C_TWENTY.SetRainbow();
      C_MINUTES.SetRainbow();
      C_TO.SetRainbow();
      break;

    // QUARTER TO
    case 9:
      currentHr++;
      C_QUARTER.SetRainbow();
      C_TO.SetRainbow();
      break;

    // TEN MINUTES TO
    case 10:
      currentHr++;
      C_TEN.SetRainbow();
      C_MINUTES.SetRainbow();
      C_TO.SetRainbow();
      break;

    // FIVE MINUTES TO
    case 11:
      currentHr++;
      C_FIVE.SetRainbow();
      C_MINUTES.SetRainbow();
      C_TO.SetRainbow();
      break;

    // N\A
    default:
      break;
  }

  switch (currentHr) {
    // ONE
    case 1:
    case 13:
      C_HR_ONE.SetRainbow();
      break;

    // TWO
    case 2:
    case 14:
      C_HR_TWO.SetRainbow();
      break;

    // THREE
    case 3:
    case 15:
      C_HR_THREE.SetRainbow();
      break;

    // FOUR
    case 4:
    case 16:
      C_HR_FOUR.SetRainbow();
      break;

    // FIVE
    case 5:
    case 17:
      C_HR_FIVE.SetRainbow();
      break;

    // SIX
    case 6:
    case 18:
      C_HR_SIX.SetRainbow();
      break;

    // SEVEN
    case 7:
    case 19:
      C_HR_SEVEN.SetRainbow();
      break;

    // EIGHT
    case 8:
    case 20:
      C_HR_EIGHT.SetRainbow();
      break;

    // NINE
    case 9:
    case 21:
      C_HR_NINE.SetRainbow();
      break;

    // TEN
    case 10:
    case 22:
      C_HR_TEN.SetRainbow();
      break;

    // ELEVEN
    case 11:
    case 23:
      C_HR_ELEVEN.SetRainbow();
      break;

    // TWELVE
    default:
      C_HR_TWELVE.SetRainbow();
      break;
  }

  // OCLOCK
  C_OCLOCK.SetRainbow();
}

// UPDATE CONTROLLERS

void updateControllers()
{
  C_IT.Update();
  C_IS.Update();
  C_TWENTY.Update();
  C_HALF.Update();
  C_QUARTER.Update();
  C_FIVE.Update();
  C_TEN.Update();
  C_HAPPY.Update();
  C_BIRTH.Update();
  C_MINUTES.Update();
  C_PAST.Update();
  C_TO.Update();
  C_HR_ONE.Update();
  C_DAY.Update();
  C_ALICE.Update();
  C_HR_FIVE.Update();
  C_HR_TWO.Update();
  C_HR_ELEVEN.Update();
  C_HR_TWELVE.Update();
  C_HR_SIX.Update();
  C_HR_FOUR.Update();
  C_HR_THREE.Update();
  C_HR_TEN.Update();
  C_HR_NINE.Update();
  C_HR_SEVEN.Update();
  C_OCLOCK.Update();
  C_HR_EIGHT.Update();
}

void clearControllers()
{
  C_IT.Clear();
  C_IS.Clear();
  C_TWENTY.Clear();
  C_HALF.Clear();
  C_QUARTER.Clear();
  C_FIVE.Clear();
  C_TEN.Clear();
  C_HAPPY.Clear();
  C_BIRTH.Clear();
  C_MINUTES.Clear();
  C_PAST.Clear();
  C_TO.Clear();
  C_HR_ONE.Clear();
  C_DAY.Clear();
  C_ALICE.Clear();
  C_HR_FIVE.Clear();
  C_HR_TWO.Clear();
  C_HR_ELEVEN.Clear();
  C_HR_TWELVE.Clear();
  C_HR_SIX.Clear();
  C_HR_FOUR.Clear();
  C_HR_THREE.Clear();
  C_HR_TEN.Clear();
  C_HR_NINE.Clear();
  C_HR_SEVEN.Clear();
  C_OCLOCK.Clear();
  C_HR_EIGHT.Clear();
}

// CALLBACKS


// SPECIAL PATTERNS

void setSpecialPattern(SpecialPattern pattern)
{
  clearControllers();

  CurrentSpecialPattern = pattern;
  SpecialPatternLastUpdate = millis();
  SpecialPatternIndex = 0;
}

void updateSpecialPattern()
{
  switch (CurrentSpecialPattern)
  {
    case SP_NONE:
      return;

    case SP_HAPPY_BIRTHDAY:
      updateHappyBirthday();
      return;
  }
}

void updateHappyBirthday()
{
  unsigned long now = millis();

  unsigned long duration = now - SpecialPatternLastUpdate;

  switch (SpecialPatternIndex)
  {
    case 0:
    case 14:
      if (duration < 2000)
      {
        return;
      }
      
      C_HAPPY.SetRainbow(4);
      break;

    case 1:
    case 15:
      if (duration < 2000)
      {
        return;
      }
      
      C_BIRTH.SetRainbow(4);
      C_DAY.SetRainbow(4);
      break;

    case 2:
    case 16:
      if (duration < 2000)
      {
        return;
      }

      C_ALICE.SetSingleColor(WC_Strip.Color(248, 24, 148));
      break;

    case 3:
    case 17:
      if (duration < 2000)
      {
        return;
      }

      clearControllers();
      break;

    case 4:
    case 18:
      if (duration < 200)
      {
        return;
      }
      
      C_HAPPY.SetRainbow(4);
      C_BIRTH.SetRainbow(4);
      C_DAY.SetRainbow(4);      
      C_ALICE.SetSingleColor(WC_Strip.Color(248, 24, 148));
      break;

    case 5:
    case 19:
      if (duration < 200)
      {
        return;
      }
      
      clearControllers();
      break;

    case 6:
    case 20:
      if (duration < 200)
      {
        return;
      }
      
      C_HAPPY.SetRainbow(4);
      C_BIRTH.SetRainbow(4);
      C_DAY.SetRainbow(4);      
      C_ALICE.SetSingleColor(WC_Strip.Color(248, 24, 148));
      break;

    case 7:
    case 21:
      if (duration < 200)
      {
        return;
      }
      
      clearControllers();
      break;

    case 8:
    case 22:
      if (duration < 200)
      {
        return;
      }
      
      C_HAPPY.SetRainbow(4);
      C_BIRTH.SetRainbow(4);
      C_DAY.SetRainbow(4);      
      C_ALICE.SetSingleColor(WC_Strip.Color(248, 24, 148));
      break;

    case 9:
    case 23:
      if (duration < 200)
      {
        return;
      }
      
      clearControllers();
      break;

    case 10:
    case 24:
      if (duration < 200)
      {
        return;
      }
      
      C_HAPPY.SetRainbow(4);
      C_BIRTH.SetRainbow(4);
      C_DAY.SetRainbow(4);      
      C_ALICE.SetSingleColor(WC_Strip.Color(248, 24, 148));
      break;

    case 11:
    case 25:
      if (duration < 200)
      {
        return;
      }
      
      clearControllers();
      break;

    case 12:
    case 26:
      if (duration < 200)
      {
        return;
      }
      
      C_HAPPY.SetRainbow(4);
      C_BIRTH.SetRainbow(4);
      C_DAY.SetRainbow(4);      
      C_ALICE.SetSingleColor(WC_Strip.Color(248, 24, 148));
      break;

    case 13:
    case 27:
      if (duration < 4000)
      {
        return;
      }

      clearControllers();
      break;

    default:
      break;
  }

  SpecialPatternIndex++;
  SpecialPatternLastUpdate = now;

  Serial.println(SpecialPatternIndex, DEC);

  if (SpecialPatternIndex >= 28)
  {
    CurrentSpecialPattern = SP_NONE;
    SpecialPatternIndex = 0;

    forceTimeUpdate();
  }
}
