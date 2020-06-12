//Cube Nightlight

#include <Adafruit_NeoPixel.h>
#include <Keypad.h>

#define LED_PIN10 10
#define LED_PIN13 13
#define LED_COUNT 3

int SLEEP_DURATION = 50;
int MAX_BRIGHTNESS = 255;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN10, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(LED_COUNT, LED_PIN13, NEO_GRB + NEO_KHZ800);

int colors[][3] = {
  /* Button Row 1 */  
  { 159, 65, 147 },  /* lighter purple - 9F4193  */
  { 0, 255, 0 },     /* green   */
  { 0, 0, 255 },     /* blue    */
  { 159, 9, 136 },   /* dark purple - 9B0988  */  

  /* Button Row 2 */ 
  { 253, 7, 210 },   /* pink      */
  { 0, 61, 101 },    /* dark blue */
  { 108, 19, 61 },   /* 6C133D    */
  { 0, 39, 55 },     /* 002737    */
 
  /* Button Row 3 */
  { 55, 0, 57 },     /* 370039    */
  { 0, 40, 13 },     /* 00280D    */
  
  { 0, 0, 0 }       /* lazy off button           */  
};

int *EXCLUSIVE_COLOR = NULL; 

const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', '4'}, /* colors 1 - 4                 */
  {'5', '6', '7', '8'}, /* colors 5 - 8                 */
  {'9', '0', 'A', 'B'}, /* colors 9, 10, OFF, ALL Colors */
  {'<', '>', '-', '+'}  /* decrease brightness, increase brightness, decrease speed, increase speed */
};

byte rowPins[ROWS] = { 6,7,8,9 }; 
byte colPins[COLS] = { 5,4,3,2 }; 

Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);

  strip.begin();                          // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();                           // Turn OFF all pixels ASAP
  strip.setBrightness(MAX_BRIGHTNESS);    // Set BRIGHTNESS to about 1/5 (max = 255)
     
  strip2.begin();                        // INITIALIZE NeoPixel strip object (REQUIRED)
  strip2.show();                         // Turn OFF all pixels ASAP
  strip2.setBrightness(MAX_BRIGHTNESS);  //(max = 255)
}

void printConcatLine(const char* mask, ...) {
    va_list params;
    va_start(params, mask);
    
    char *ptr = (char *)mask;
    while (*ptr != '\0') {
        if (*ptr == 'c') {
            int c = va_arg(params, int);
            Serial.write(c);
        } else if (*ptr == 'i') {
            int i = va_arg(params, int);
            Serial.print(i);
        } else if (*ptr == 's') {
            const char *s = va_arg(params, const char *);
            Serial.print(s);
        } else if (*ptr == 'f' || *ptr == 'd') {
            // Be careful with this. It's not portable. On AVR float
            // and double are the same, but that's not the case on other
            // microcontrollers. It would be better to split them.
            double d = va_arg(params, double);
            Serial.print(d);
        } else {
            Serial.write(*ptr);
        }
        ptr++;
    }
    
    va_end(params);
    Serial.println();
}

bool processColor(int* color) {
  printConcatLine("sisisis", "Color: {", color[0], ", ", color[1], ", ", color[2], "}");
  
  for(int brightness = 1; brightness <= MAX_BRIGHTNESS; ++brightness) {
    bool success = dimmerRangeCycle(color, brightness);
    if(!success) {
      return false;  
    }
  }
  
  for(int brightness = MAX_BRIGHTNESS; brightness >= 1; --brightness) {  
    bool success = dimmerRangeCycle(color, brightness);
    if(!success) {
      return false;  
    }
  }
  return true;
}

bool dimmerRangeCycle(int* color, int brightness) {
  int resp;
  
  colorWipe(strip.Color(color[0], color[1], color[2]), 0, brightness, strip);
  colorWipe(strip2.Color(color[0], color[1], color[2]), 0, brightness, strip2);
  
  resp = eventButtonPress();
  if(resp == 2) {
    //exclusive color set
    return false;      
  }
  
  delay(SLEEP_DURATION);
  return true;
}

void loop() { 
  if(EXCLUSIVE_COLOR) {
    Serial.println("EXCLUSIVE_COLOR");
    processColor(EXCLUSIVE_COLOR);
  } else {
    //loop through all colors  
    for (int x = 0; x < 11; x++ ) {
      if(!(colors[x][0] == 255 && colors[x][1] == 255 && colors[x][2] == 255)) {       
        bool success = processColor(colors[x]);
        if(!success) x = 12;
      }  
    }
  }
}

// Some functions of our own for creating animated effects -----------------

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait, int brightness, Adafruit_NeoPixel& strip) {
  strip.setBrightness(brightness);   
  
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
  }
}

/* int - Returns 1: on success. */
/*       Returns 2: if EXCLUSIVE_COLOR is set and successful. */

int eventButtonPress() {
  char btnPressed = keypad.getKey();
  
  if (btnPressed) {
    switch(btnPressed) {
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':        
        EXCLUSIVE_COLOR = colors[(int) btnPressed - 49];
        return 2;
        
        break;
        
      case 0:
        EXCLUSIVE_COLOR = colors[9];      
        return 2;
        break;
        
      case 'A':
        EXCLUSIVE_COLOR = colors[10];  
        return 2;
        break;
        
      case 'B':
        EXCLUSIVE_COLOR = NULL;  
        return 2;
        break;
        
      case '<':
        decreaseBrightness();
        break;
        
      case '>':
        increaseBrightness();        
        break;

      case '-':
        decreaseSpeed();
        break;
        
      case '+':
        increaseSpeed();
        break;
    }  
    Serial.println(btnPressed);
  }
  return 1;
}

void increaseSpeed() {
  SLEEP_DURATION -= 5;
  if(SLEEP_DURATION < 0) SLEEP_DURATION = 0;
  printConcatLine("ci", "Sleep Duration: ", SLEEP_DURATION);
}

void decreaseSpeed() {
  SLEEP_DURATION += 5;
  printConcatLine("ci", "Sleep Duration: ", SLEEP_DURATION);
}

void increaseBrightness() {
  MAX_BRIGHTNESS += 5;
  if(MAX_BRIGHTNESS > 255) MAX_BRIGHTNESS = 255;

  printConcatLine("ci", "Max Brightness: ", MAX_BRIGHTNESS);
}

void decreaseBrightness() {
  MAX_BRIGHTNESS -= 5;
  if(MAX_BRIGHTNESS < 5) MAX_BRIGHTNESS = 5;
  printConcatLine("ci", "Max Brightness: ", MAX_BRIGHTNESS);
}

