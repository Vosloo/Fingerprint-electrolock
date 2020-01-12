#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Key.h>
#include <Keypad.h>
#include <SoftwareSerial.h>

#define RELAY 10

const byte ROWS = 4; 
const byte COLS = 3; 

char hexaKeys[ROWS][COLS] = 
{
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte arrowUp[] = {
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100
};

byte arrowDown[] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110,
  B00100
};

const int pageCount = 4;

byte rowPins[ROWS] = {3, 4, 5, 6}; 
byte colPins[COLS] = {7, 8, 9}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

const long timeForDisplayToTurnOff = 8000;
boolean backlightOn = false;
String displayedMsg = "";
long timeElapsed = -1;

String tmpString = "";
boolean tmpFlag = false;

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial mySerial(12, 13);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void switchBacklight(){
  if(backlightOn)
  {
    lcd.noBacklight();
    backlightOn = false;
  }
  else
  {
    lcd.backlight();
    backlightOn = true;
  }
}

boolean checkIfKeyIsInState(char querryKey, KeyState state){
  if(customKeypad.getKeys())
  {
    for (int i = 0; i < LIST_MAX; i++) 
    {
      char customKey = customKeypad.key[i].kchar;
      if(customKey == '\0')
      {
        continue;
      }
      if(customKey != querryKey)
      {
        continue;
      }

      if(customKeypad.key[i].stateChanged)
      {
        if(customKeypad.key[i].kstate == state)
        {
          return true;
        }
      }
    }
  }
  return false;
}

boolean screenActivated(){
  boolean activated = checkIfKeyIsInState('*', HOLD);
  return activated;
}

void emptyDatabase(){
  lcd.clear();
  lcd.print("Do You want to");
  lcd.setCursor(0, 1);
  lcd.print("delete fingers?");
  while(true)
  {
    char keyPressed = customKeypad.waitForKey();
    if(keyPressed != '*' && keyPressed != '#')
    {
      continue;
    }
    if(keyPressed == '*')
    {
      lcd.clear();
      lcd.print("Action aborted.");
      break;
    }
    else
    {
      lcd.clear();
      finger.emptyDatabase();
      lcd.print("Database cleared");
      delay(2000);
      break;
    }
  }
}

void clearLine(int lineNum){
  for(int i = 0; i < 16; i++)
  {
    lcd.setCursor(i, lineNum);
    lcd.print(" ");
  }
  lcd.setCursor(0, 0);
}

void showLegend(){ // Make switching pages slicker;
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.write(0);
  lcd.print('*');
  lcd.setCursor(14, 1);
  lcd.print('#');
  lcd.write(1);
  lcd.setCursor(0, 0);

  String functionLabels[] = {"kupa", "kupa1", "kupa2", "kupa3"};
  int currentPage = 0;
  lcd.print(functionLabels[currentPage]);
  while(!checkIfKeyIsInState('0', HOLD))
  {
    char keyPressed = customKeypad.getKey();
    if(keyPressed)
    {
      switch (keyPressed)
      {
      case '*':
        if(currentPage > 0)
        {
          clearLine(0);
          currentPage -= 1;
          lcd.print(functionLabels[currentPage]);
        }
        break;
      
      case '#':
        if(currentPage < pageCount - 1)
        {
          clearLine(0);
          currentPage += 1;
          lcd.print(functionLabels[currentPage]);
        }
        break;
      }
    }
  }
}

void switchDisplayOff(){
  timeElapsed = -1;
  displayedMsg = "";
  tmpFlag = false;
  digitalWrite(RELAY, HIGH);
  lcd.clear();
  switchBacklight();
}

void setup(){
  Serial.begin(9600);
  finger.begin(57600);

  lcd.init();
  lcd.begin(16, 2);
  lcd.createChar(0, arrowDown);
  lcd.createChar(1, arrowUp);
  lcd.clear();

  if(finger.verifyPassword()) 
  {
    switchBacklight();
    lcd.print("  Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print(" is now online!");
    delay(3000);
    lcd.clear();
    switchBacklight();
  } 
  else 
  {
    switchBacklight();
    lcd.print("  Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("   not found!");
    while(1);
  }

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);

  customKeypad.setHoldTime(1000); // Sets the amount of milliseconds the user will have to hold a button until the HOLD state is triggered.
}
  
void loop(){
  String msg = "";

  if(!backlightOn && screenActivated())
  {
    timeElapsed = millis();
    switchBacklight();
  }

  if(backlightOn && checkIfKeyIsInState('0', HOLD)) // Legend for actions;
  {
    showLegend();
    lcd.clear();
    timeElapsed = millis();
  }
  
  if (backlightOn && customKeypad.getKeys())
  {
    for (int i = 0; i < LIST_MAX; i++)   // Scan the whole key list.
    {
      if(customKeypad.key[i].stateChanged)   // Only find keys that have changed state.
      {
        char customKey = customKeypad.key[i].kchar;
        KeyState state;

        switch (customKeypad.key[i].kstate) 
        {
          case PRESSED:
            state = PRESSED;
            break;
          case HOLD:
            state = HOLD;
        }

        switch (customKey)
        {
          case '1':
            lcd.clear();
            lcd.print("Add new finger.");
            delay(100);
            break;
          case '2':
            lcd.clear();
            lcd.print("Delete finger.");
            delay(100);
            break;
          case '3':
            emptyDatabase();
            break;
          default:
            lcd.clear();
            lcd.print("This button has");
            lcd.setCursor(0, 1);
            lcd.print("no action!");
            delay(100);
            break;
        }
      }
    }
    timeElapsed = millis(); // No action;
  }

  if(!tmpFlag && timeElapsed != -1 && millis() - timeElapsed >= timeForDisplayToTurnOff) // Wait for 8 secs before turning screen off
  {
    switchDisplayOff();
  }
}