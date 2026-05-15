Plant -Vitals  (Andrew Lim, Andrew Valdivia, Angel Leon)
ca#include <LiquidCrystal.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <Servo.h>


const int ANGLE_CLOSED = 70;  
const int ANGLE_OPEN = 160;


// --- CALIBRATION ADJUSTED FOR CUP OF WATER ---
const int RAW_DRY = 50;    // Standard air value
const int RAW_WET = 600;   // Increased to catch the signal in a cup of water


const int SOIL_PIN = A1;    
const int THERM_PIN = A0;    
const int LIGHT_PIN = A5;    
const int SERVO_PIN = 10;
const int CATEGORY_BUTTON_PIN = 6;
const int MODE_BUTTON_PIN = 7;
const int LOG_BUTTON_PIN = 9;


LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Servo wateringServo;


int currentCategory = 0;
int currentPlant = 1;
float soilFilt = 0;
float tempF = 0;
int lightLevel = 0;
bool isWateringNow = false;
const float ALPHA = 0.95;


enum OperatingMode { SELECT_PLANT, LOGGING, SUMMARY };
OperatingMode currentMode = SELECT_PLANT;


const char* plantNames[] = {"Tall Fescue", "K. Blue", "Zoysia", "Cactus", "Aloe Vera", "Carrot", "Pine Tree", "Avocado", "Olive Tree"};
const int DRY_LIMITS[] = {35, 40, 30, 10, 20, 50, 40, 30, 20};
const char* categoryNames[] = {"GRASS", "PLANT", "TREE"};
const int categoryMap[][4] = {{0,1,2,-1}, {3,4,5,-1}, {6,7,8,-1}};


unsigned long prevLogMillis = 0;
unsigned long prevLcdMillis = 0;
unsigned long summaryStartMillis = 0;
int statCycle = 0;


void readAllSensors() {
  int sRaw = analogRead(SOIL_PIN);
 
  // Debug to Serial Monitor so you can see why it's not triggering
  Serial.print("Raw Soil: "); Serial.println(sRaw);


  int sPct = map(sRaw, RAW_DRY, RAW_WET, 0, 100);
  sPct = constrain(sPct, 0, 100);
  soilFilt = (ALPHA * sPct) + ((1.0 - ALPHA) * soilFilt);


  int tRaw = analogRead(THERM_PIN);
  if (tRaw < 1) tRaw = 1;
  float resistance = (1023.0 / (float)tRaw) - 1.0;
  resistance = 10000.0 / resistance;
  float tempC = log(resistance / 10000.0) / 3950.0;
  tempC = 1.0 / (tempC + (1.0 / (25.0 + 273.15))) - 273.15;
  tempF = (tempC * 9.0 / 5.0) + 32.0;


  lightLevel = map(analogRead(LIGHT_PIN), 0, 1023, 0, 100);
}


void handleAutoWatering() {
  // RULE: Only categories 1 (Plant) and 2 (Tree) move the pipe
  if ((currentCategory == 1 || currentCategory == 2) && currentMode == LOGGING) {
    if (soilFilt < DRY_LIMITS[currentPlant]) {
      wateringServo.write(ANGLE_OPEN);
      isWateringNow = true;
    } else {
      wateringServo.write(ANGLE_CLOSED);
      isWateringNow = false;
    }
  } else {
    wateringServo.write(ANGLE_CLOSED);
    isWateringNow = false;
  }
}


void showSummary() {
  lcd.clear();
  lcd.setCursor(0,0);
  String warns = "";
  if (soilFilt < 30) warns += "DRY ";
  if (tempF > 85)    warns += "HOT ";
  if (lightLevel < 30) warns += "DARK";
  if (warns == "") lcd.print("STATUS: STABLE");
  else lcd.print(warns);


  lcd.setCursor(0,1);
  if (tempF > 85 && lightLevel > 70) lcd.print("REC: CACTUS");
  else if (tempF < 60) lcd.print("REC: PINE TREE");
  else if (soilFilt > 60) lcd.print("REC: CARROTS");
  else lcd.print("REC: TALL FESCUE");
}


void checkButtons() {
  if (digitalRead(CATEGORY_BUTTON_PIN) == LOW) {
    currentCategory = (currentCategory + 1) % 3;
    currentPlant = categoryMap[currentCategory][0];
    lcd.clear(); lcd.print("CAT: "); lcd.print(categoryNames[currentCategory]);
    delay(500);
  }
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    static int pIdx = 0; pIdx++;
    if (categoryMap[currentCategory][pIdx] == -1) pIdx = 0;
    currentPlant = categoryMap[currentCategory][pIdx];
    lcd.clear(); lcd.print("PLANT: "); lcd.print(plantNames[currentPlant]);
    delay(500);
  }
  if (digitalRead(LOG_BUTTON_PIN) == LOW) {
    if (currentMode == SELECT_PLANT) {
      currentMode = LOGGING;
    } else if (currentMode == LOGGING) {
      currentMode = SUMMARY;
      summaryStartMillis = millis();
      showSummary();
    }
    delay(500);
  }
}


void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  wateringServo.attach(SERVO_PIN);
  wateringServo.write(ANGLE_CLOSED);
  pinMode(CATEGORY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LOG_BUTTON_PIN, INPUT_PULLUP);
}


void loop() {
  checkButtons();
  unsigned long now = millis();


  if (now - prevLogMillis >= 200) {
    prevLogMillis = now;
    readAllSensors();
    handleAutoWatering();
  }


  if (currentMode == LOGGING) {
    if (now - prevLcdMillis >= 1500) {
      prevLcdMillis = now;
      lcd.clear();
      lcd.setCursor(0,0);
     
      // DIAGNOSTIC FEEDBACK
      if (isWateringNow) lcd.print("PIPE: OPEN");
      else lcd.print("PIPE: CLOSED");
     
      lcd.setCursor(0,1);
      if (statCycle == 0) { lcd.print("SOIL: "); lcd.print((int)soilFilt); lcd.print("%"); statCycle = 1; }
      else if (statCycle == 1) { lcd.print("TEMP: "); lcd.print((int)tempF); lcd.print("F"); statCycle = 2; }
      else { lcd.print("LIGHT: "); lcd.print(lightLevel); lcd.print("%"); statCycle = 0; }
    }
  }
  else if (currentMode == SUMMARY) {
    if (now - summaryStartMillis >= 20000) {
      currentMode = SELECT_PLANT;
      lcd.clear();
    }
  }
  else {
    if (now - prevLcdMillis >= 2000) {
      prevLcdMillis = now;
      lcd.clear();
      lcd.print(plantNames[currentPlant]);
      lcd.setCursor(0,1); lcd.print("PRESS LOG START");
    }
  }
}


const int ANGLE_CLOSED = 70;  
const int ANGLE_OPEN = 160;


// --- CALIBRATION ADJUSTED FOR CUP OF WATER ---
const int RAW_DRY = 50;    // Standard air value
const int RAW_WET = 600;   // Increased to catch the signal in a cup of water


const int SOIL_PIN = A1;    
const int THERM_PIN = A0;    
const int LIGHT_PIN = A5;    
const int SERVO_PIN = 10;
const int CATEGORY_BUTTON_PIN = 6;
const int MODE_BUTTON_PIN = 7;
const int LOG_BUTTON_PIN = 9;


LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Servo wateringServo;


int currentCategory = 0;
int currentPlant = 1;
float soilFilt = 0;
float tempF = 0;
int lightLevel = 0;
bool isWateringNow = false;
const float ALPHA = 0.95;


enum OperatingMode { SELECT_PLANT, LOGGING, SUMMARY };
OperatingMode currentMode = SELECT_PLANT;


const char* plantNames[] = {"Tall Fescue", "K. Blue", "Zoysia", "Cactus", "Aloe Vera", "Carrot", "Pine Tree", "Avocado", "Olive Tree"};
const int DRY_LIMITS[] = {35, 40, 30, 10, 20, 50, 40, 30, 20};
const char* categoryNames[] = {"GRASS", "PLANT", "TREE"};
const int categoryMap[][4] = {{0,1,2,-1}, {3,4,5,-1}, {6,7,8,-1}};


unsigned long prevLogMillis = 0;
unsigned long prevLcdMillis = 0;
unsigned long summaryStartMillis = 0;
int statCycle = 0;


void readAllSensors() {
  int sRaw = analogRead(SOIL_PIN);
 
  // Debug to Serial Monitor so you can see why it's not triggering
  Serial.print("Raw Soil: "); Serial.println(sRaw);


  int sPct = map(sRaw, RAW_DRY, RAW_WET, 0, 100);
  sPct = constrain(sPct, 0, 100);
  soilFilt = (ALPHA * sPct) + ((1.0 - ALPHA) * soilFilt);


  int tRaw = analogRead(THERM_PIN);
  if (tRaw < 1) tRaw = 1;
  float resistance = (1023.0 / (float)tRaw) - 1.0;
  resistance = 10000.0 / resistance;
  float tempC = log(resistance / 10000.0) / 3950.0;
  tempC = 1.0 / (tempC + (1.0 / (25.0 + 273.15))) - 273.15;
  tempF = (tempC * 9.0 / 5.0) + 32.0;


  lightLevel = map(analogRead(LIGHT_PIN), 0, 1023, 0, 100);
}


void handleAutoWatering() {
  // RULE: Only categories 1 (Plant) and 2 (Tree) move the pipe
  if ((currentCategory == 1 || currentCategory == 2) && currentMode == LOGGING) {
    if (soilFilt < DRY_LIMITS[currentPlant]) {
      wateringServo.write(ANGLE_OPEN);
      isWateringNow = true;
    } else {
      wateringServo.write(ANGLE_CLOSED);
      isWateringNow = false;
    }
  } else {
    wateringServo.write(ANGLE_CLOSED);
    isWateringNow = false;
  }
}


void showSummary() {
  lcd.clear();
  lcd.setCursor(0,0);
  String warns = "";
  if (soilFilt < 30) warns += "DRY ";
  if (tempF > 85)    warns += "HOT ";
  if (lightLevel < 30) warns += "DARK";
  if (warns == "") lcd.print("STATUS: STABLE");
  else lcd.print(warns);


  lcd.setCursor(0,1);
  if (tempF > 85 && lightLevel > 70) lcd.print("REC: CACTUS");
  else if (tempF < 60) lcd.print("REC: PINE TREE");
  else if (soilFilt > 60) lcd.print("REC: CARROTS");
  else lcd.print("REC: TALL FESCUE");
}


void checkButtons() {
  if (digitalRead(CATEGORY_BUTTON_PIN) == LOW) {
    currentCategory = (currentCategory + 1) % 3;
    currentPlant = categoryMap[currentCategory][0];
    lcd.clear(); lcd.print("CAT: "); lcd.print(categoryNames[currentCategory]);
    delay(500);
  }
  if (digitalRead(MODE_BUTTON_PIN) == LOW) {
    static int pIdx = 0; pIdx++;
    if (categoryMap[currentCategory][pIdx] == -1) pIdx = 0;
    currentPlant = categoryMap[currentCategory][pIdx];
    lcd.clear(); lcd.print("PLANT: "); lcd.print(plantNames[currentPlant]);
    delay(500);
  }
  if (digitalRead(LOG_BUTTON_PIN) == LOW) {
    if (currentMode == SELECT_PLANT) {
      currentMode = LOGGING;
    } else if (currentMode == LOGGING) {
      currentMode = SUMMARY;
      summaryStartMillis = millis();
      showSummary();
    }
    delay(500);
  }
}


void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  wateringServo.attach(SERVO_PIN);
  wateringServo.write(ANGLE_CLOSED);
  pinMode(CATEGORY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LOG_BUTTON_PIN, INPUT_PULLUP);
}


void loop() {
  checkButtons();
  unsigned long now = millis();


  if (now - prevLogMillis >= 200) {
    prevLogMillis = now;
    readAllSensors();
    handleAutoWatering();
  }


  if (currentMode == LOGGING) {
    if (now - prevLcdMillis >= 1500) {
      prevLcdMillis = now;
      lcd.clear();
      lcd.setCursor(0,0);
     
      // DIAGNOSTIC FEEDBACK
      if (isWateringNow) lcd.print("PIPE: OPEN");
      else lcd.print("PIPE: CLOSED");
     
      lcd.setCursor(0,1);
      if (statCycle == 0) { lcd.print("SOIL: "); lcd.print((int)soilFilt); lcd.print("%"); statCycle = 1; }
      else if (statCycle == 1) { lcd.print("TEMP: "); lcd.print((int)tempF); lcd.print("F"); statCycle = 2; }
      else { lcd.print("LIGHT: "); lcd.print(lightLevel); lcd.print("%"); statCycle = 0; }
    }
  }
  else if (currentMode == SUMMARY) {
    if (now - summaryStartMillis >= 20000) {
      currentMode = SELECT_PLANT;
      lcd.clear();
    }
  }
  else {
    if (now - prevLcdMillis >= 2000) {
      prevLcdMillis = now;
      lcd.clear();
      lcd.print(plantNames[currentPlant]);
      lcd.setCursor(0,1); lcd.print("PRESS LOG START");
    }
  }
}

