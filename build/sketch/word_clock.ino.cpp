#include <Arduino.h>
#line 1 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
#line 1 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_NeoPixel.h>

#define TIME_CHECK_INTERVAL 10000
#define LED_PIN 6
#define LED_COUNT 120
#define LED_BRIGHTNESS 86
#define BUTTON_PIN 8
#define BDAY_MONTH 5
#define BDAY_DAY 3

// CONTROLLERS

enum ControllerPattern
{
    NONE,
    SINGLE_COLOR,
    HUE_CYCLE,
    RAINBOW_CYCLE,
    THEATER_CHASE,
    COLOR_WIPE,
    SCANNER,
    BOX_ZOOM,
    SNAKE
};

enum PatternDirection
{
    FORWARD,
    REVERSE
};

struct ColorCombo
{
    uint32_t Color1;
    uint32_t Color2;
};

class Controller
{
public:
    static uint16_t GlobalTick;
    static unsigned long LastUpdate; // last update of position

    // Member Variables:
    ControllerPattern ActivePattern = NONE; // which pattern is running
    PatternDirection Direction = FORWARD;   // direction to run the pattern

    uint16_t Interval; // milliseconds between updates
    uint16_t TickDuration;

    uint32_t Color1, Color2; // What colors are in use
    uint16_t TotalSteps;     // total number of steps in the pattern
    uint16_t Index;          // current step within the pattern

    uint8_t MaxLoops;
    uint8_t LoopIndex;

    Adafruit_NeoPixel &NeoPixel;

    const uint8_t StartPixel;
    const uint8_t EndPixel;

    void (*OnComplete)(ControllerPattern completedPattern); // Callback on completion of pattern

    Controller(Adafruit_NeoPixel &neoPixel, const uint8_t startPixel, const uint8_t endPixel, void (*callback)(ControllerPattern completedPattern)) : NeoPixel(neoPixel), StartPixel(startPixel), EndPixel(endPixel)
    {
        OnComplete = callback;
    }

    static void UpdateTick()
    {
        unsigned long now = millis();

        Controller::GlobalTick = now - Controller::LastUpdate;
        Controller::LastUpdate = now;
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

        TickDuration += Controller::GlobalTick;

        if (TickDuration < Interval)
        {
            return;
        }

        TickDuration = 0;

        switch (ActivePattern)
        {
            case HUE_CYCLE:
                HueCycleUpdate();
                return;

            case RAINBOW_CYCLE:
                RainbowCycleUpdate();
                return;

            case THEATER_CHASE:
                TheaterChaseUpdate();
                return;

            case COLOR_WIPE:
                ColorWipeUpdate();
                return;

            case SCANNER:
                ScannerUpdate();
                return;

            case BOX_ZOOM:
                BoxZoomUpdate();
                return;

            case SNAKE:
                SnakeUpdate();
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

            if (Index >= TotalSteps)
            {
                Index = 0;

                CheckCompletion();
            }
        }
        else
        {
            --Index;

            if (Index <= 0)
            {
                Index = TotalSteps - 1;

                CheckCompletion();
            }
        }
    }

    void CheckCompletion()
    {
        if (MaxLoops == 0)
        {
            return;
        }

        LoopIndex++;

        if (LoopIndex < MaxLoops)
        {
            return;
        }

        ControllerPattern completedPattern = ActivePattern;

        Clear();

        if (OnComplete != NULL)
        {
            OnComplete(completedPattern);
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
        TickDuration = 0;

        Color1 = 0;
        Color2 = 0;

        LoopIndex = 0;
        MaxLoops = 0;

        ColorSet(NeoPixel.Color(0, 0, 0));
    }

    void SetSingleColor(uint32_t color)
    {
        ActivePattern = SINGLE_COLOR;
        Interval = 0;
        TotalSteps = 0;
        Index = 0;
        Direction = FORWARD;

        Color1 = color;
        Color2 = 0;

        LoopIndex = 0;
        MaxLoops = 0;

        ColorSet(color);
    }

    void HueCycle(uint16_t interval = 40, PatternDirection dir = FORWARD, uint8_t loops = 0)
    {
        ActivePattern = HUE_CYCLE;
        Interval = interval;
        TotalSteps = 256;
        Index = 0;
        Direction = dir;

        Color1 = random(256);
        Color2 = 0;

        LoopIndex = 0;
        MaxLoops = loops;
    }

    void HueCycleUpdate()
    {
        ColorSet(Wheel((Color1 + Index) % 256));
        Increment();
    }

    // Initialize for a RainbowCycle
    void RainbowCycle(uint16_t interval = 10, PatternDirection dir = FORWARD, uint8_t loops = 0)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        TotalSteps = 255;
        Index = 0;
        Direction = dir;

        Color1 = 0;
        Color2 = 0;

        LoopIndex = 0;
        MaxLoops = loops;
    }

    // Update the Rainbow Cycle Pattern
    void RainbowCycleUpdate()
    {
        uint8_t numPixels = GetNumPixels();
        uint8_t x = 0;

        for (uint8_t i = StartPixel; i <= EndPixel; i++)
        {
            NeoPixel.setPixelColor(i, Wheel(((x * 256 / numPixels) + Index) & 255));
            x++;
        }

        NeoPixel.show();
        Increment();
    }

    // Initialize for a Theater Chase
    void TheaterChase(uint16_t interval = 10, uint32_t color1 = 0, uint32_t color2 = 0, PatternDirection dir = FORWARD, uint8_t loops = 0)
    {
        ActivePattern = THEATER_CHASE;
        Interval = interval;
        TotalSteps = GetNumPixels();
        Index = 0;
        Direction = dir;

        if (color1 == 0 && color2 == 0)
        {
            ColorCombo combo = GetRandomColors();

            Color1 = combo.Color1;
            Color2 = combo.Color2;
        }
        else
        {
            Color1 = color1;
            Color2 = color2;
        }

        LoopIndex = 0;
        MaxLoops = loops;
    }

    // Update the Theater Chase Pattern
    void TheaterChaseUpdate()
    {
        for (uint8_t i = StartPixel; i <= EndPixel; i++)
        {
            if ((i + Index) % 3 == 0)
            {
                NeoPixel.setPixelColor(i, Color1);
            }
            else
            {
                NeoPixel.setPixelColor(i, Color2);
            }
        }

        NeoPixel.show();
        Increment();
    }

    // Initialize for a ColorWipe
    void ColorWipe(uint16_t interval = 10, uint32_t color1 = 0, uint32_t color2 = 0, PatternDirection dir = FORWARD, uint8_t loops = 0)
    {
        ActivePattern = COLOR_WIPE;
        Interval = interval;
        TotalSteps = GetNumPixels() * 2;
        Index = 0;
        Direction = dir;

        if (color1 == 0 && color2 == 0)
        {
            ColorCombo combo = GetRandomColors();

            Color1 = combo.Color1;
            Color2 = combo.Color2;
        }
        else
        {
            Color1 = color1;
            Color2 = color2;
        }

        LoopIndex = 0;
        MaxLoops = loops;
    }

    // Update the Color Wipe Pattern
    void ColorWipeUpdate()
    {
        uint8_t half = TotalSteps / 2;

        if (Index < half)
        {
            NeoPixel.setPixelColor(StartPixel + Index, Color1);
        }
        else
        {
            NeoPixel.setPixelColor(StartPixel + half - (Index % half), Color2);
        }

        NeoPixel.show();

        Increment();
    }

    // Initialize for a SCANNNER
    void Scanner(uint16_t interval, uint32_t color1 = 0, uint32_t color2 = 0, uint8_t loops = 0)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = GetNumPixels();
        Index = 0;
        Direction = FORWARD;

        if (color1 == 0 && color2 == 0)
        {
            ColorCombo combo = GetRandomColors();

            Color1 = combo.Color1;
            Color2 = combo.Color2;
        }
        else
        {
            Color1 = color1;
            Color2 = color2;
        }

        LoopIndex = 0;
        MaxLoops = loops;
    }

    // Update the Scanner Pattern
    void ScannerUpdate()
    {
        uint16_t halfway = TotalSteps / 2;

        uint8_t pixel1;
        uint8_t pixel2;

        if (Index < halfway)
        {
            pixel1 = Index;
            pixel2 = EndPixel - Index;
        }
        else
        {
            pixel1 = halfway - (Index % halfway) - 1;
            pixel2 = halfway + (Index % halfway);
        }

        for (uint8_t i = StartPixel; i <= EndPixel; i++)
        {
            if (i == pixel1)
            {
                NeoPixel.setPixelColor(i, Color1);
            }
            else if (i == pixel2)
            {
                NeoPixel.setPixelColor(i, Color2);
            }
            else // Fading tail
            {
                NeoPixel.setPixelColor(i, DimColor(NeoPixel.getPixelColor(i)));
            }
        }

        NeoPixel.show();
        Increment();
    }

    void BoxZoom(uint16_t interval = 500, uint32_t color1 = 0, uint32_t color2 = 0, uint8_t loops = 0)
    {
        if (GetNumPixels() < 120)
        {
            return;
        }

        ActivePattern = BOX_ZOOM;
        Interval = interval;
        TotalSteps = 10;
        Index = 0;
        Direction = FORWARD;

        if (color1 == 0 && color2 == 0)
        {
            ColorCombo combo = GetRandomColors();

            Color1 = combo.Color1;
            Color2 = combo.Color2;
        }
        else
        {
            Color1 = color1;
            Color2 = color2;
        }

        LoopIndex = 0;
        MaxLoops = loops;
    }

    void BoxZoomUpdate()
    {
        const uint8_t *pixels;
        uint8_t plength;

        switch (Index)
        {
            case 0:
            case 9:
                pixels = new uint8_t[40]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 35, 36, 59, 60, 83, 84, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 96, 95, 72, 71, 48, 47, 24, 23};
                plength = 40;
                break;

            case 1:
            case 8:
                pixels = new uint8_t[32]{13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 46, 49, 70, 73, 94, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 85, 82, 61, 58, 37, 34};
                plength = 32;
                break;

            case 2:
            case 7:
                pixels = new uint8_t[24]{26, 27, 28, 29, 30, 31, 32, 33, 38, 57, 62, 81, 86, 87, 88, 89, 90, 91, 92, 93, 74, 69, 50, 45};
                plength = 24;
                break;

            case 3:
            case 6:
                pixels = new uint8_t[16]{39, 40, 41, 42, 43, 44, 51, 68, 75, 76, 77, 78, 79, 80, 63, 56};
                plength = 16;
                break;

            case 4:
            case 5:
                pixels = new uint8_t[8]{52, 53, 54, 55, 64, 65, 66, 67};
                plength = 8;
                break;
        }

        uint32_t color;

        if (Index >= 5)
        {
            color = NeoPixel.Color(0, 0, 0);
        }
        else if (Index % 2 == 0)
        {
            color = Color1;
        }
        else
        {
            color = Color2;
        }

        for (uint8_t i = 0; i < plength; i++)
        {
            NeoPixel.setPixelColor(pixels[i], color);
        }

        delete[] pixels;

        NeoPixel.show();
        Increment();
    }

    void Snake(uint16_t interval = 21, uint8_t loops = 0)
    {
        if (GetNumPixels() < 120)
        {
            return;
        }

        ActivePattern = SNAKE;
        Interval = interval;
        TotalSteps = 240;
        Index = 0;
        Direction = FORWARD;

        Color1 = 0;
        Color2 = 0;

        LoopIndex = 0;
        MaxLoops = loops;
    }

    void SnakeUpdate()
    {
        static const uint8_t pixels[120] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 23, 24, 47, 48, 71, 72, 95, 96, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 84, 83, 60, 59, 36, 35, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 25, 46, 49, 70, 73, 94, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 85, 82, 61, 58, 37, 34, 33, 32, 31, 30, 29, 28, 27, 26, 45, 50, 69, 74, 93, 92, 91, 90, 89, 88, 87, 86, 81, 62, 57, 38, 39, 40, 41, 42, 43, 44, 51, 68, 75, 76, 77, 78, 79, 80, 63, 56, 55, 54, 53, 52, 67, 66, 65, 64};

        uint8_t i;
        uint32_t color;

        if (Index >= 120)
        {
            i = 120 - (Index % 120) - 1;
            color = NeoPixel.Color(0, 0, 0);
        }
        else
        {
            i = Index;
            color = NeoPixel.ColorHSV(Index * 65535 / 119);
        }

        NeoPixel.setPixelColor(pixels[i], color);

        NeoPixel.show();
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
    uint32_t Wheel(uint8_t pos)
    {
        uint16_t hue = pos * 65535 / 255;

        return NeoPixel.ColorHSV(hue);
    }

    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        return NeoPixel.Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
    }

    uint32_t GetRandomColor()
    {
        uint16_t hue = random(65536);

        return NeoPixel.ColorHSV(hue);
    }

    ColorCombo GetRandomColors()
    {
        uint16_t hue1 = random(65536);
        uint16_t hue2 = (hue1 + 32768) % 65536;

        ColorCombo combo;

        combo.Color1 = NeoPixel.ColorHSV(hue1);
        combo.Color2 = NeoPixel.ColorHSV(hue2);

        return combo;
    }

    uint8_t GetNumPixels()
    {
        return EndPixel - StartPixel + 1;
    }
};

class WordController
{
public:
    ControllerPattern ActivePattern = NONE;

    uint8_t Interval;
    uint8_t TickDuration;

    uint8_t Index;
    uint8_t Seed;

    Adafruit_NeoPixel &NeoPixel;

    const uint8_t StartPixel;
    const uint8_t EndPixel;

    WordController(Adafruit_NeoPixel &neoPixel, const uint8_t startPixel, const uint8_t endPixel) : NeoPixel(neoPixel), StartPixel(startPixel), EndPixel(endPixel)
    {
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

        TickDuration += Controller::GlobalTick;

        if (TickDuration < Interval)
        {
            return;
        }

        TickDuration = 0;

        switch (ActivePattern)
        {
            case HUE_CYCLE:
                HueCycleUpdate();
                return;

            case RAINBOW_CYCLE:
                RainbowCycleUpdate();
                return;
        }
    }

    void Increment()
    {
        Index++;

        if (Index >= 256)
        {
            Index = 0;
        }
    }

    void Clear()
    {
        if (ActivePattern == NONE)
        {
            return;
        }

        ActivePattern = NONE;
        Interval = 0;
        Index = 0;
        TickDuration = 0;

        ColorSet(NeoPixel.Color(0, 0, 0));
    }

    void HueCycle(uint16_t interval = 40)
    {
        ActivePattern = HUE_CYCLE;
        Interval = interval;
        Index = 0;
        Seed = random(256);
    }

    void HueCycleUpdate()
    {
        ColorSet(Wheel((Seed + Index) % 256));
        Increment();
    }

    void RainbowCycle(uint16_t interval = 4, PatternDirection dir = FORWARD, uint8_t loops = 0)
    {
        ActivePattern = RAINBOW_CYCLE;
        Interval = interval;
        Index = 0;
        Seed = 0;
    }

    void RainbowCycleUpdate()
    {
        uint8_t numPixels = GetNumPixels();
        uint8_t x = 0;

        for (uint8_t i = StartPixel; i <= EndPixel; i++)
        {
            NeoPixel.setPixelColor(i, Wheel(((x * 256 / numPixels) + Index) & 255));
            x++;
        }

        NeoPixel.show();
        Increment();
    }

    void SetSingleColor(uint32_t color)
    {
        ActivePattern = SINGLE_COLOR;
        Index = 0;
        Seed = 0;

        ColorSet(color);
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

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(uint8_t pos)
    {
        uint16_t hue = pos * 65535 / 255;

        return NeoPixel.ColorHSV(hue);
    }

    uint8_t GetNumPixels()
    {
        return EndPixel - StartPixel + 1;
    }
};

unsigned long Controller::LastUpdate;
uint16_t Controller::GlobalTick;

// INIT

RTC_DS3231 rtc;

bool ButtonPressed = false;
unsigned long ButtonPressTime = 0;
uint8_t ButtonTick = 0;

unsigned long LastTimeCheck = 32768;
uint8_t LastHr = 255;
uint8_t LastMin = 255;
uint8_t TimeHrOffset = 0;

enum SpecialPattern
{
    SP_NONE,
    SP_DEMO,
    SP_LIGHT_SHOW,
    SP_HAPPY_BIRTHDAY
};

SpecialPattern CurrentSpecialPattern = SP_NONE;

unsigned long SpecialPatternLastUpdate = 0;
uint8_t SpecialPatternIndex = 0;

// INIT NEOPIXEL
Adafruit_NeoPixel WC_Strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void onPatternComplete(ControllerPattern completedPattern);

Controller C_ALL(WC_Strip, 0, 119, &onPatternComplete);

WordController C_TWENTY(WC_Strip, 0, 5);
WordController C_IS(WC_Strip, 7, 8);
WordController C_IT(WC_Strip, 10, 11);
WordController C_QUARTER(WC_Strip, 12, 18);
WordController C_HALF(WC_Strip, 20, 23);
WordController C_HAPPY(WC_Strip, 24, 28);
WordController C_TEN(WC_Strip, 29, 31);
WordController C_FIVE(WC_Strip, 32, 35);
WordController C_MINUTES(WC_Strip, 36, 42);
WordController C_BIRTH(WC_Strip, 43, 47);
WordController C_DAY(WC_Strip, 48, 50);
WordController C_HR_ONE(WC_Strip, 51, 53);
WordController C_TO(WC_Strip, 55, 56);
WordController C_PAST(WC_Strip, 56, 59);
WordController C_HR_TWO(WC_Strip, 60, 62);
WordController C_HR_FIVE(WC_Strip, 63, 66);
WordController C_ALICE(WC_Strip, 67, 71);
WordController C_HR_TWELVE(WC_Strip, 72, 77);
WordController C_HR_ELEVEN(WC_Strip, 78, 83);
WordController C_HR_THREE(WC_Strip, 84, 88);
WordController C_HR_FOUR(WC_Strip, 89, 92);
WordController C_HR_SIX(WC_Strip, 93, 95);
WordController C_HR_TEN(WC_Strip, 96, 98);
WordController C_HR_NINE(WC_Strip, 99, 102);
WordController C_HR_SEVEN(WC_Strip, 103, 107);
WordController C_HR_EIGHT(WC_Strip, 108, 112);
WordController C_OCLOCK(WC_Strip, 114, 119);

// BEGIN EXECUTION

#line 873 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void setup();
#line 899 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void loop();
#line 909 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void checkButton();
#line 925 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void onButtonUp();
#line 942 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void onButtonDown();
#line 989 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void toggleLightShow();
#line 1005 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
DateTime getTimeWithOffset();
#line 1019 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void checkTime();
#line 1076 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void forceTimeUpdate();
#line 1086 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void updateTime(uint8_t currentHr, uint8_t currentMin);
#line 1277 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void updateControllers();
#line 1310 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void clearControllers();
#line 1343 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void setSpecialPattern(SpecialPattern pattern);
#line 1364 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void pickRandomDemo();
#line 1400 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void updateSpecialPattern();
#line 1418 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void clearSpecialPattern(bool setTime);
#line 1480 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void updateHappyBirthday();
#line 873 "c:\\Users\\ryank\\Documents\\Arduino\\word_clock\\word_clock.ino"
void setup()
{
    Serial.begin(19200);

    Serial.println("Starting...");

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
        {
        };
    }

    if (rtc.lostPower())
    {
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
        if (ButtonPressed == true)
        {
            onButtonUp();
        }
        return;
    }

    // BUTTON IS PRESSED
    onButtonDown();
}

void onButtonUp()
{
    unsigned long duration = millis() - ButtonPressTime;

    if (duration > 50)
    {
        Serial.println("Button UP");

        ButtonPressed = false;

        if (duration < 2000)
        {
            toggleLightShow();
        }
    }
}

void onButtonDown()
{
    unsigned long now = millis();

    if (ButtonPressed == false)
    {
        Serial.println("Button DOWN");

        ButtonPressed = true;
        ButtonTick = 0;
        ButtonPressTime = now;

        return;
    }

    unsigned long duration = now - ButtonPressTime;

    // BUTTON PRESSED FOR AT LEAST 2 SECONDS
    if (duration < 2000)
    {
        return;
    }

    // 1 SECOND UPDATE INTERVAL
    uint8_t tick = (duration / 1000) + 1;

    if (tick == ButtonTick)
    {
        return;
    }

    ButtonTick = tick;

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

void toggleLightShow()
{
    switch (CurrentSpecialPattern)
    {
        case NONE:
            setSpecialPattern(SP_LIGHT_SHOW);
            return;

        default:
            clearSpecialPattern(true);
            return;
    }
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
    if ((now - LastTimeCheck) < TIME_CHECK_INTERVAL)
    {
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

    if (currentHr == 21 && currentHr != LastHr && LastHr != 255)
    {
        setSpecialPattern(SP_LIGHT_SHOW);
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

void updateTime(uint8_t currentHr, uint8_t currentMin)
{
    Serial.print("Updating the time to ");
    Serial.print(currentHr, DEC);
    Serial.print(':');
    Serial.print(currentMin, DEC);
    Serial.println();

    LastHr = currentHr;
    LastMin = currentMin;

    uint8_t currentMinInterval = currentMin / 5;

    // CLEAR SPECIAL PATTERN
    clearSpecialPattern(false);

    // CLEAR ALL CONTROLLERS
    clearControllers();

    // IT IS
    C_IT.HueCycle();
    C_IS.HueCycle();

    switch (currentMinInterval)
    {
        // FIVE MINUTES PAST
        case 1:
            C_FIVE.HueCycle();
            C_MINUTES.HueCycle();
            C_PAST.HueCycle();
            break;

        // TEN MINUTES PAST
        case 2:
            C_TEN.HueCycle();
            C_MINUTES.HueCycle();
            C_PAST.HueCycle();
            break;

        // QUARTER PAST
        case 3:
            C_QUARTER.HueCycle();
            C_PAST.HueCycle();
            break;

        // TWENTY MINUTES PAST
        case 4:
            C_TWENTY.HueCycle();
            C_MINUTES.HueCycle();
            C_PAST.HueCycle();
            break;

        // TWENTY FIVE MINUTES PAST
        case 5:
            C_TWENTY.HueCycle();
            C_FIVE.HueCycle();
            C_MINUTES.HueCycle();
            C_PAST.HueCycle();
            break;

        // HALF PAST
        case 6:
            C_HALF.HueCycle();
            C_PAST.HueCycle();
            break;

        // TWENTY FIVE MINUTES TO
        case 7:
            currentHr++;
            C_TWENTY.HueCycle();
            C_FIVE.HueCycle();
            C_MINUTES.HueCycle();
            C_TO.HueCycle();
            break;

        // TWENTY MINUTES TO
        case 8:
            currentHr++;
            C_TWENTY.HueCycle();
            C_MINUTES.HueCycle();
            C_TO.HueCycle();
            break;

        // QUARTER TO
        case 9:
            currentHr++;
            C_QUARTER.HueCycle();
            C_TO.HueCycle();
            break;

        // TEN MINUTES TO
        case 10:
            currentHr++;
            C_TEN.HueCycle();
            C_MINUTES.HueCycle();
            C_TO.HueCycle();
            break;

        // FIVE MINUTES TO
        case 11:
            currentHr++;
            C_FIVE.HueCycle();
            C_MINUTES.HueCycle();
            C_TO.HueCycle();
            break;

        // N\A
        default:
            break;
    }

    switch (currentHr)
    {
        // ONE
        case 1:
        case 13:
            C_HR_ONE.HueCycle();
            break;

        // TWO
        case 2:
        case 14:
            C_HR_TWO.HueCycle();
            break;

        // THREE
        case 3:
        case 15:
            C_HR_THREE.HueCycle();
            break;

        // FOUR
        case 4:
        case 16:
            C_HR_FOUR.HueCycle();
            break;

        // FIVE
        case 5:
        case 17:
            C_HR_FIVE.HueCycle();
            break;

        // SIX
        case 6:
        case 18:
            C_HR_SIX.HueCycle();
            break;

        // SEVEN
        case 7:
        case 19:
            C_HR_SEVEN.HueCycle();
            break;

        // EIGHT
        case 8:
        case 20:
            C_HR_EIGHT.HueCycle();
            break;

        // NINE
        case 9:
        case 21:
            C_HR_NINE.HueCycle();
            break;

        // TEN
        case 10:
        case 22:
            C_HR_TEN.HueCycle();
            break;

        // ELEVEN
        case 11:
        case 23:
            C_HR_ELEVEN.HueCycle();
            break;

        // TWELVE
        default:
            C_HR_TWELVE.HueCycle();
            break;
    }

    // OCLOCK
    C_OCLOCK.RainbowCycle();
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

    Controller::UpdateTick();
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

// SPECIAL PATTERNS

void setSpecialPattern(SpecialPattern pattern)
{
    clearControllers();
    C_ALL.Clear();

    CurrentSpecialPattern = pattern;
    SpecialPatternLastUpdate = millis();
    SpecialPatternIndex = 0;

    switch (pattern)
    {
        case SP_DEMO:
            pickRandomDemo();
            return;

        case SP_LIGHT_SHOW:
            C_ALL.HueCycle(4, FORWARD, 10);
            return;
    }
}

void pickRandomDemo()
{
    uint8_t i = random(7);

    switch (i)
    {
        case 0:
            C_ALL.HueCycle(4, FORWARD, 10);
            return;

        case 1:
            C_ALL.RainbowCycle(8, FORWARD, 5);
            return;

        case 2:
            C_ALL.TheaterChase(100, 0, 0, FORWARD, 1);
            return;

        case 3:
            C_ALL.ColorWipe(8, 0, 0, FORWARD, 5);
            return;

        case 4:
            C_ALL.Scanner(30, 0, 0, 4);
            return;

        case 5:
            C_ALL.BoxZoom(500, 0, 0, 3);
            return;

        case 6:
            C_ALL.Snake(21, 4);
            return;
    }
}

void updateSpecialPattern()
{
    switch (CurrentSpecialPattern)
    {
        case SP_NONE:
            return;

        case SP_DEMO:
        case SP_LIGHT_SHOW:
            C_ALL.Update();
            return;

        case SP_HAPPY_BIRTHDAY:
            updateHappyBirthday();
            return;
    }
}

void clearSpecialPattern(bool setTime)
{
    if (CurrentSpecialPattern == SP_NONE)
    {
        return;
    }

    CurrentSpecialPattern = SP_NONE;
    SpecialPatternIndex = 0;

    C_ALL.Clear();

    if (setTime)
    {
        forceTimeUpdate();
    }
}

void onPatternComplete(ControllerPattern completedPattern)
{
    if (CurrentSpecialPattern != SP_LIGHT_SHOW)
    {
        clearSpecialPattern(true);
        return;
    }

    switch (completedPattern)
    {
        case HUE_CYCLE:
            C_ALL.RainbowCycle(8, FORWARD, 5);
            return;

        case RAINBOW_CYCLE:
            C_ALL.TheaterChase(100, 0, 0, FORWARD, 1);
            return;

        case THEATER_CHASE:
            C_ALL.ColorWipe(8, 0, 0, FORWARD, 5);
            return;

        case COLOR_WIPE:
            C_ALL.Scanner(30, 0, 0, 4);
            return;

        case SCANNER:
            C_ALL.BoxZoom(500, 0, 0, 3);
            return;

        case BOX_ZOOM:
            C_ALL.Snake(21, 4);
            return;

        case SNAKE:
            clearSpecialPattern(true);
            return;

        default:
            clearSpecialPattern(true);
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

            C_HAPPY.RainbowCycle();
            break;

        case 1:
        case 15:
            if (duration < 2000)
            {
                return;
            }

            C_BIRTH.RainbowCycle();
            C_DAY.RainbowCycle();
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

            C_HAPPY.RainbowCycle();
            C_BIRTH.RainbowCycle();
            C_DAY.RainbowCycle();
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

            C_HAPPY.RainbowCycle();
            C_BIRTH.RainbowCycle();
            C_DAY.RainbowCycle();
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

            C_HAPPY.RainbowCycle();
            C_BIRTH.RainbowCycle();
            C_DAY.RainbowCycle();
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

            C_HAPPY.RainbowCycle();
            C_BIRTH.RainbowCycle();
            C_DAY.RainbowCycle();
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

            C_HAPPY.RainbowCycle();
            C_BIRTH.RainbowCycle();
            C_DAY.RainbowCycle();
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

    if (SpecialPatternIndex >= 28)
    {
        clearSpecialPattern(true);
    }
}

