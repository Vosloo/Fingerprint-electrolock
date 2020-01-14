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

bool createFingerprintModel(uint8_t ID){
  int status = -1;

  status = finger.createModel();
  if (status == FINGERPRINT_OK) 
  {
    lcd.clear();
    lcd.print("Dopasowano");
    lcd.setCursor(0, 1);
    lcd.print("odcisk palca!");
    delay(1000);
  } 
  else if (status == FINGERPRINT_PACKETRECIEVEERR) 
  {
    lcd.clear();
    lcd.print("Comm error!");
    delay(4000);
    return false;
  } 
  else if (status == FINGERPRINT_ENROLLMISMATCH) 
  {
    lcd.clear();
    lcd.print("Niedopasowano!");
    delay(2500);
    return false;
  } 
  else 
  {
    lcd.clear();
    lcd.print("Unknown error!");
    delay(4000);
    return false;
  }   
  
  status = finger.storeModel(ID);
  if (status == FINGERPRINT_OK) 
  {
    lcd.clear();
    lcd.print("Zapisano nowy");
    lcd.setCursor(0, 1);
    lcd.print("odcisk palca!");
    delay(1000);
    return true;
  } 
  else if (status == FINGERPRINT_PACKETRECIEVEERR) 
  {
    lcd.clear();
    lcd.print("Comm error!");
    delay(4000);
    return false;
  } 
  else if (status == FINGERPRINT_BADLOCATION) 
  {
    lcd.clear();
    lcd.print("Bad storage");
    lcd.setCursor(0, 1);
    lcd.print("location!");
    delay(4000);
    return false;
  } 
  else if (status == FINGERPRINT_FLASHERR) 
  {
    lcd.clear();
    lcd.print("Error writing");
    lcd.setCursor(0, 1);
    lcd.print("to flash!");
    delay(4000);
    return false;
  } 
  else 
  {
    lcd.clear();
    lcd.print("Unknown error!");
    delay(4000);
    return false;
  }   
}

void waitForNoFinger(){
  int status = 0;
  while (status != FINGERPRINT_NOFINGER) 
  {
    status = finger.getImage();
  }
}

bool convertFingerprint(bool again=false){
  int status = -1;

  if(again)
  {
    status = finger.image2Tz(2);
  }
  else
  {
    status = finger.image2Tz(1);
  }
  
  switch (status) 
  {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.print("Przetworzono!");
      delay(2000);
      return true;
    case FINGERPRINT_IMAGEMESS:
      lcd.clear();
      lcd.print("Obraz");
      lcd.setCursor(0, 1);
      lcd.print("nieczytelny!");
      delay(3000);
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Comm error!");
      delay(4000);
      return false;
    case FINGERPRINT_FEATUREFAIL:
      lcd.clear();
      lcd.print("Fingerprint");
      lcd.setCursor(0, 1);
      lcd.print("features error!");
      delay(3000);
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      lcd.clear();
      lcd.print("Fingerprint");
      lcd.setCursor(0, 1);
      lcd.print("features error!");
      delay(3000);
      return false;
    default:
      lcd.clear();
      lcd.print("Unknown error!");
      delay(4000);
      return false;
  }
}

bool scanFingerprint(bool again=false){
  int status = -1;
  lcd.clear();
  if(again)
  {
    lcd.print("Przyloz palec");
    lcd.setCursor(0, 1);
    lcd.print("ponownie.");
  }
  else
  {
    lcd.print("Przyloz palec.");
  }
  while (status != FINGERPRINT_OK) 
  {
    status = finger.getImage();
    switch (status) 
    {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.print("Przetwarzanie");
      return true;
    case FINGERPRINT_NOFINGER: // just wait
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      lcd.clear();
      lcd.print("Comm error!");
      delay(4000);
      return false;
    case FINGERPRINT_IMAGEFAIL:
      lcd.clear();
      lcd.print("Ponow probe.");
      delay(2000);
      break;
    default:
      lcd.clear();
      lcd.print("Unknown error!");
      delay(4000);
      return false;
    }
  }
}

bool checkIfIDTaken(uint8_t id){
  uint8_t model = finger.loadModel(id);
  switch (model) 
  {
  case FINGERPRINT_OK:
    return true;
  case FINGERPRINT_PACKETRECIEVEERR:
    return true; // Should never occur
  default:
    return false;
  }
}

bool checkBoundaries(int number){
  if(number > 0 && number < 128)
  {
    return true;
  }
  else
  {
    return false;
  }
}

int convertToInt(String number){
  int numInt = number.toInt();
  return numInt;
}

uint8_t getID() {
  uint8_t num = 0;
  String IDMessage = "(1-127) ID: ";
  String number = "";

  lcd.clear();
  lcd.print("Podaj ID odcisku");
  lcd.setCursor(0, 1);
  lcd.print(IDMessage); //12 kolumna zaczyna się; max = 15
  while (true)
  {
    char keyPressed = customKeypad.waitForKey();
    int numLength = number.length();
    if(keyPressed == '#') // Accepted ID
    {
      int tempNumber = convertToInt(number);
      if(tempNumber == 0) // Couldnt convert
      {
        number = "";
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print("Podano zle ID!");
        delay(2500);
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print(IDMessage);
        continue;
      }

      bool success = checkBoundaries(tempNumber);
      if(success)
      {
        num = (uint8_t)tempNumber;
        bool IDTaken = checkIfIDTaken(num);
        if(IDTaken)
        {
          number = "";
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print("ID zajete!");
          delay(2500);
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print(IDMessage);
        }
        else
        {
          return num;
        }
      }
      else
      {
        number = "";
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print("Mozliwe ID:1-127");
        delay(2500);
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print(IDMessage);
      }
    }
    else if(keyPressed == '*' && numLength >= 1) // Delete one character
    {
      number.remove(numLength - 1);
      clearLine(1);
      lcd.setCursor(0, 1);
      lcd.print(IDMessage + number);
    }
    else if(keyPressed == '0' && number.length() == 0) // Exit 
    {
      lcd.clear();
      lcd.print("Przerwano.");
      return 255; // Max value for byte - error num as cannot be returned as ID;
    }
    else if(number.length() != 4)
    {
      number.concat(keyPressed);
      lcd.print(keyPressed);
    }
  }
}

void addFingerprint(){
  uint8_t ID = getID();
  if(ID == 255) // Error ID (aborted)
  {
    return;
  }

  bool success;
  success = scanFingerprint();
  if(success)
  {
    success = convertFingerprint();
    if(success)
    {
      lcd.clear();
      lcd.print("Odsun palec.");
      delay(1500);
      waitForNoFinger();
      success = scanFingerprint(true);
      if(success)
      {
        success = convertFingerprint(true);
        if(success)
        {
          lcd.clear();
          lcd.print("Odsun palec.");
          delay(1500);
          waitForNoFinger();
          createFingerprintModel(ID);
        }
      }
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
  lcd.print(" 0 to quit.");
  lcd.setCursor(14, 1);
  lcd.print('#');
  lcd.write(1);
  lcd.setCursor(0, 0);

  boolean exitLegend = false;
  String functionLabels[] = {"kupa", "kupa1", "kupa2", "kupa3"};
  int currentPage = 0;
  lcd.print(functionLabels[currentPage]);
  while(true)
  {
    char keyPressed = customKeypad.getKey();
    if(keyPressed)
    {
      switch (keyPressed)
      {
      case '0':
        exitLegend = true;
        break;

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

      if(exitLegend)
      {
        lcd.clear();
        return;
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
    delay(100);
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
            break;
        }

        switch (customKey)
        {
          case '0':
            if(state == PRESSED)
            {
              lcd.clear();
              lcd.print("Hold to see");
              lcd.setCursor(0, 1);
              lcd.print("action legend.");
              break;
            }
            else if(state != HOLD)
            {
              break;
            }
            showLegend();
            state = RELEASED;
            break;

          case '1':
            addFingerprint();
            state = RELEASED;
            break;

          case '2':
            lcd.clear();
            lcd.print("Delete finger.");
            break;

          case '3':
            emptyDatabase();
            break;

          case '*':
            break; //no action

          case '#':
            break; //no action

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