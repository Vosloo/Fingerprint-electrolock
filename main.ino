#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Key.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

//LCD is at SDA, SCL (Could be A4 and A5); FINGERPRINT: 11 - green, 12 - white; RELAY is at 10; KEYBOARD: ROWS - 3, 4, 5, 6 and COLUMNS - 7, 8, 9 

#define RELAY 10

unsigned long PASSWORD;
long PASSADDRESS = 0; 

const byte ROWS = 4; 
const byte COLS = 3; 

char hexaKeys[ROWS][COLS] = {
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

byte rowPins[ROWS] = {3, 4, 5, 6}; 
byte colPins[COLS] = {7, 8, 9}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

const long timeForDisplayToTurnOff = 8000;
boolean backlightOn = false;
String displayedMsg = "";
long timeElapsed = -1;

boolean passOk = false;

String tmpString = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial mySerial(11, 12); //green, white

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void writeToEEPROM(int address, unsigned long value){
	byte four = (value & 0xFF);
	byte three = ((value >> 8) & 0xFF);
	byte two = ((value >> 16) & 0xFF);
	byte one = ((value >> 24) & 0xFF);
	
	EEPROM.update(address, four);
	EEPROM.update(address + 1, three);
	EEPROM.update(address + 2, two);
	EEPROM.update(address + 3, one);
}

unsigned long readFromEEPROM(long address){
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  if(one == 255 && two == 255 && three == 255 && four == 255)
  {
    return 255;
  }

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

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
  lcd.print("Czy chcesz");
  lcd.setCursor(0, 1);
  lcd.print("usunac odciski?");
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
      lcd.print("Przerwano.");
      break;
    }
    else
    {
      lcd.clear();
      finger.emptyDatabase();
      lcd.print("Usunieto");
      lcd.setCursor(0, 1);
      lcd.print("odciski!");
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

bool searchFingerprint(){
  int status = -1;
  
  while (status != FINGERPRINT_OK) 
  {
    status = finger.getImage();

    char keyPressed = customKeypad.getKey();
    if(keyPressed == '0')
    {
      lcd.clear();
      lcd.print("Przerwano!");
      return false;
    }
    
    switch (status) 
    {
    case FINGERPRINT_OK:
      lcd.clear();
      lcd.print("Przetwarzanie");
      break;
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
    
  status = finger.image2Tz();
  if(status == FINGERPRINT_PACKETRECIEVEERR)
  {
    lcd.clear();
    lcd.print("Comm error!");
    delay(4000);
    return false;
  }
  else if (status != FINGERPRINT_OK)
  {
    return false;
  }

  status = finger.fingerFastSearch();
  if (status == FINGERPRINT_OK) 
  {
    lcd.clear();
    lcd.print("Dopasowano!");
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
  else if (status == FINGERPRINT_NOTFOUND) 
  {
    lcd.clear();
    lcd.print("Niedopasowano!");
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

bool matchFingerprint(){
  lcd.clear();
  lcd.print("Przyloz palec");

  bool success = searchFingerprint();
  return success;
}

bool checkIfIDTaken(uint8_t id){
  uint8_t model = finger.loadModel(id);
  switch (model) 
  {
  case FINGERPRINT_OK:
    return true;
  case FINGERPRINT_PACKETRECIEVEERR:
    return false; // Should never occur
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

unsigned long convertToIntLong(String number){
  unsigned long numInt = number.toInt();
  return numInt;
}

uint8_t getID(bool deleteID=false) {
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
      int tempNumber = convertToIntLong(number);
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
        if(deleteID)
        {
          if(IDTaken)
          {
            return num;
          }
          else
          {
            number = "";
            clearLine(1);
            lcd.setCursor(0, 1);
            lcd.print("Brak takiego ID");
            delay(2500);
            clearLine(1);
            lcd.setCursor(0, 1);
            lcd.print(IDMessage);
          }
        }
        else
        {
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

void deleteInDatabase(uint8_t ID){
  uint8_t status = -1;
  
  status = finger.deleteModel(ID);

  if (status == FINGERPRINT_OK) 
  {
    lcd.clear();
    lcd.print("Usunieto odcisk");
  } 
  else if (status == FINGERPRINT_PACKETRECIEVEERR) 
  {
    lcd.clear();
    lcd.print("Comm error!");
    delay(4000);
  } 
  else if (status == FINGERPRINT_BADLOCATION) 
  {
    lcd.clear();
    lcd.print("Bad storage");
    lcd.setCursor(0, 1);
    lcd.print("location!");
    delay(4000);
  } 
  else if (status == FINGERPRINT_FLASHERR) 
  {
    lcd.clear();
    lcd.print("Error writing");
    lcd.setCursor(0, 1);
    lcd.print("to flash!");
    delay(4000);
  } 
  else 
  {
    lcd.clear();
    lcd.print("Unknown error!");
    delay(4000);
  }   
}

void deleteID(){
  lcd.clear();
  lcd.print("Usuwanie palca!");
  delay(1500);
  uint8_t ID = getID(true);
  if(ID == 255) // Error ID (aborted)
  {
    return;
  }

  deleteInDatabase(ID);
}

void addFingerprint(){
  lcd.clear();
  lcd.print("Dodawanie palca!");
  delay(1500);
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

void showLegend(){
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
  String functionLabels[] = {"1-Dodaj odcisk", "2-Usun odcisk", "3-Usuwanie bazy", "5-Otwarcie zamka", "8-Zmiana hasla"}; //See if it works
  int pageCount = sizeof(functionLabels) / sizeof(functionLabels[0]);
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
  passOk = false;
  digitalWrite(RELAY, HIGH);
  lcd.clear();
  switchBacklight();
}

void openLock(){
  digitalWrite(RELAY, LOW);
  lcd.clear();
  lcd.print("Otwarto zamek");
  delay(3000);
  digitalWrite(RELAY, HIGH);
  lcd.clear();
  delay(10);
  lcd.print("Zamknieto zamek");
}

bool checkIfPassCorrect(unsigned long number){
  if(((String)number).length() != 8)
  {
    return false;
  }
  Serial.println(PASSWORD);
  if(PASSWORD != number)
  {
    return false;
  }
  return true;
}

bool verifyPassword(){
  String number = "";
  lcd.clear();
  lcd.print("Podaj aktualne");
  lcd.setCursor(0, 1);
  String passMessage = "haslo: ";
  lcd.print(passMessage);

  while (true)
  {
    char keyPressed = customKeypad.waitForKey();
    int numLength = number.length();
    if(keyPressed == '#') // Accepted ID
    {
      unsigned long tempNumber = convertToIntLong(number);
      Serial.println(tempNumber);
      if(tempNumber == 0) // Couldnt convert
      {
        number = "";
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print("Not a number!");
        delay(2500);
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print(passMessage);
        continue;
      }

      bool success = checkIfPassCorrect(tempNumber);
      if(success)
      {
        lcd.clear();
        lcd.print("Zweryfikowano!");
        delay(500);
        passOk = true;
        return true;
      }
      else
      {
        lcd.clear();
        lcd.print("Bledne haslo!");
        return false;
      }
    }
    else if(keyPressed == '*' && numLength >= 1) // Delete one character
    {
      number.remove(numLength - 1);
      clearLine(1);
      lcd.setCursor(0, 1);
      lcd.print(passMessage + number);
    }
    else if(keyPressed == '0' && number.length() == 0) // Exit 
    {
      lcd.clear();
      lcd.print("Przerwano.");
      return false; // Max value for byte - error num as cannot be returned as ID;
    }
    else if(number.length() != 8)
    {
      number.concat(keyPressed);
      lcd.print(keyPressed);
    }
  }
}

void changePassword(){
  bool verified = verifyPassword();

  if(passOk && verified)
  {
    lcd.clear();
    lcd.print("Podaj nowe");
    lcd.setCursor(0, 1);
    String passMessage = "haslo: ";
    lcd.print(passMessage);
    String number = "";

    while (true)
    {
      char keyPressed = customKeypad.waitForKey();
      int numLength = number.length();
      if(keyPressed == '#') // Accepted ID
      {
        unsigned long tempNumber = convertToIntLong(number);
        if(tempNumber == 0) // Couldnt convert
        {
          number = "";
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print("Not a number!");
          delay(2500);
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print(passMessage);
          continue;
        }

        if(number.length() != 8)
        {
          number = "";
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print("Dlugosc =/= 8");
          delay(2500);
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print(passMessage);
          continue;
        }
        writeToEEPROM(PASSADDRESS, tempNumber);
        PASSWORD = tempNumber;
        lcd.clear();
        lcd.print("Zaktualizowano");
        lcd.setCursor(0, 1);
        lcd.print("haslo!");
        return;
      }
      else if(keyPressed == '*' && numLength >= 1) // Delete one character
      {
        number.remove(numLength - 1);
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print(passMessage + number);
      }
      else if(keyPressed == '0' && number.length() == 0) // Exit 
      {
        lcd.clear();
        lcd.print("Przerwano.");
        return; // Max value for byte - error num as cannot be returned as ID;
      }
      else if(number.length() != 8)
      {
        number.concat(keyPressed);
        lcd.print(keyPressed);
      }
    }
  }
}

void setup(){
  Serial.begin(9600);
  finger.begin(57600);

  PASSWORD = readFromEEPROM(PASSADDRESS); // reads password from eeprom;
  // Serial.println(PASSWORD);
  if(PASSWORD == 255) //nothing set yet
  {
    PASSWORD = 12345678; //default Password = 12345678
  }
  // Serial.println(PASSWORD);
  
  lcd.init();
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
  
  if (customKeypad.getKeys())
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
        
        if(!backlightOn)
        {
          if(customKey == '*')
          {
            switchBacklight();
            timeElapsed = millis();
            state = RELEASED;
          }
        }

        if(backlightOn)
        {
          switch (customKey)
          {
            case '0':
              if(state == PRESSED)
              {
                lcd.clear();
                lcd.print("Przytrzymaj by");
                lcd.setCursor(0, 1);
                lcd.print("zobaczyc legende");
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
              if(!passOk)
                verifyPassword();
              if(passOk)
                addFingerprint();
              state = RELEASED;
              break;

            case '2':
              if(!passOk)
                verifyPassword();
              if(passOk)
                deleteID();
              state = RELEASED;
              break;

            case '3':
              if(!passOk)
                verifyPassword();
              if(passOk)
                emptyDatabase();
              state = RELEASED;
              break;

            case '5':
              if(state == PRESSED)
              {
                bool matched = matchFingerprint();
                if(matched)
                {
                  openLock();
                }
                state = RELEASED;
              }
              break;

            case '8':
              changePassword();
              state = RELEASED;
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
              break;
          }
        }
      }
    }
    timeElapsed = millis(); // No action;
  }

  if(timeElapsed != -1 && millis() - timeElapsed >= timeForDisplayToTurnOff) // Wait for 8 secs before turning screen off
  {
    switchDisplayOff();
  }
}