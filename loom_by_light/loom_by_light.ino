#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include <LiquidCrystal.h>

/**
Required libraries: 
  LiquidCrystal by Arduino, Adafruit -version last tested: 1.0.7. 
  Adafruit NeoPixel by Adafruit -version last tested: 1.12.4, 
  
This sketch allows loading of a bitmap image through an SD card to have each row of the 
bitmap displayed on an addressable LED strip as either off or on.

*The purpose of this is for a custom loom, which allows the user to manually choose which 
of 2 colours to use on each loom heddle, thereby creating custom 2d loom images based off 
a bitmap.

LCD button controls:
SELECT - toggles between configure mode, and row mode.
up / down - iterates through rows, configure settings, and file navigation.
right - selects the file to load in file navigator.
left / right - changes the values in the current configure.

Note: the bitmap to be opened must be 24 bits per pixel and not compressed.

*The program is written by Luke Johnson, commissioned by Elizabeth Johnson, 
the organizer of the project.
this version of Loom by Light: 1.6.2
*some of the code was modified after being sourced from bitsnbytes.co.uk:
https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*@author Luke Johnson
*@date 2025-February-03
*/

#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>


#define CHIP_SELECT 10 //the chip select used for the SD card.
//the maximum number of LED's that can be configured to be allowed on a light strip,
// the amount of LEDs is configured by pressing select in the program.
#define LED_COUNT 144 
#define LED_PIN 1 //the pin the data line for the addressable LED strip is connected to.
// NeoPixel brightness, 0 (min) to 255 (max)
#define LCD_ROWS 2 //the number of character rows on the lcd screen, this is how many lines fit on the lcd screen.
#define LCD_COLS 16 //the number of character columns on the lcd screen, this is how many characters fit on one line.
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //the pin values for the lcd display.
//this is the amount the brightness changes when changing the brightness in the config menu.
#define BRIGHTNESS_INCREMENT 1

//EEPROM memory address locations:
#define EEPROM_ROW 0 //the memory location in the EEPROM for the current row.
#define EEPROM_BRIGHTNESS 2 //the memory location in the EEPROM for the brigthness.
#define EEPROM_OFFSET 3 //the memory location in the EEPROM for the led offset.
#define EEPROM_LED_COUNT 5
#define NUM_BYTES_PER_PIXEL 3 //the number of bytes per a pixel in a bitmap.

//to turn on debug, set DO_DEBUG to 1, else Serial debug messages will not show.
#define DO_DEBUG 0

#if DO_DEBUG == 1
#define DEBUG_BEGIN Serial.begin(9600)
#define DEBUG_MSG(x) Serial.print(x)
#define DEBUG_LN(x) Serial.println(x)
#define DEBUG_WR(x) Serial.write(x)
#else
#define DEBUG_BEGIN
#define DEBUG_MSG(x)
#define DEBUG_LN(X)
#define DEBUG_WR(x)
#endif

//global variables:
uint8_t brightness;
int ledOffset;
int ledCount = (int)LED_COUNT;
LiquidCrystal * lcd;
uint8_t stateInt = 0;



//lcd display variables
long timeAtLcdReset;
#define LCD_CYCLE_DURATION 1000
#define LCD_SHORT_MESSAGE_DURATION 1000
#define LCD_MED_MESSAGE_DURATION 2000
uint8_t charCount = 0; //the character count in the message string.

/**
display a message at row.
*/
void displayMsgAtRow(String message, int row) {
  lcd->setCursor(0,row);
  lcd->print(message);
}

/**
display a message, waiting a duration before continuing, if a button is pressed
it breaks the loop
*/
void resetAndDisplayMessageWithBreakableLoopLcd(String message, int timeAmount) {
  resetAndDisplayStringLcd(message);
  long timeWhenMessageLoopStarted = millis();
  while(millis() < timeWhenMessageLoopStarted + timeAmount) {
    readButtons();
    displayStringLcd(message);
    if (isAnyButtonPressed()) {
      break;
    }
  }
}

/**
reset the lcd display and display the string to the lcd.
*/
void resetAndDisplayStringLcd(String message) {
  lcd->clear();
  timeAtLcdReset = millis();
  charCount = 0;
  displayStringLcd(message);
}



/**
display the string to the lcd display. It uses the timer timeAtLcdReset to know 
when to progress to the next phase of the message.
*/
void displayStringLcd(String message) {
  //loop vars
  int row = 0;
  int col = 0;
  int charCountInsideLoop = charCount;
  //if timer for lcd is above threshold
  if (millis() >= timeAtLcdReset + LCD_CYCLE_DURATION) {
    //increase charCount by LCD_ROWS * LCD_COLS
    charCountInsideLoop += (LCD_ROWS * LCD_COLS);
    //if char count inside loop bigger than size of message string.
    if (charCountInsideLoop >= message.length()) {
      //set char count inside loop to 0 to reset lcd message start.
      charCountInsideLoop = 0;
    }
    //reset the lcd
    lcd->clear();
  }
  //only loop while withing row and column bounds and within character bounds for chars
  //within this loop.
  while ((row < LCD_ROWS) && (col < LCD_COLS) && (charCountInsideLoop < message.length())) {
    //write this row
    lcd->setCursor(col, row); //set the position on the lcd
    lcd->write((char)message[charCountInsideLoop]); //write the lcd character at postion
    charCountInsideLoop ++; //increment the char count inside this loop.
    col ++; //increment the column
    if (col >= LCD_COLS) {//if at max columns for this row
      col = 0;//set column to 0
      row ++;//iterate row
    }
    //lcd cycle finished, display the lcd.
    if (millis() >= timeAtLcdReset + LCD_CYCLE_DURATION) {
      timeAtLcdReset = millis();
      lcd->display();//display the lcd
    }

  }
}

//buttons from lcd input
int _adcKeyIn; //the key input 
bool _upPressed = false;
bool _downPressed = false;
bool _leftPressed = false;
bool _rightPressed = false;
bool _selectPressed = false;
bool _hasInputBeenRead = false; //this is set true when an input is read, and used to determine
//when pressed bools should be set from true to false in the _isPressesd functions.


void readButtons() {
  _adcKeyIn = analogRead(0);
  if (_adcKeyIn > 1000) {
    if (_hasInputBeenRead) { //if input has been read, set the pressed bools to false.
      _upPressed = false;
      _downPressed = false;
      _leftPressed = false;
      _rightPressed = false;
      _selectPressed = false;
      _hasInputBeenRead = false; //also set the bool to false so another button press can be read.
    }
    return;
  }
  if (_adcKeyIn < 50) {
    if (!_hasInputBeenRead) { //used to keep a button from repeating when held down.
      _rightPressed = true;
    }
    return; 
  }
  if (_adcKeyIn < 250) {
    if (!_hasInputBeenRead) {//used to keep a button from repeating when held down.
      _upPressed = true;
    }
    return;
  } 
  if (_adcKeyIn < 450) {
    if (!_hasInputBeenRead) { //used to keep a button from repeating when held down.
      _downPressed = true;
    }
    return;
  }
  if (_adcKeyIn < 650) {
    if (!_hasInputBeenRead) { //used to keep a button from repeating when held down.
      _leftPressed = true;  
    }
    return;
  }
  if (_adcKeyIn < 850) {
    if (!_hasInputBeenRead) { //used to keep a button from repeating when held down.
      _selectPressed = true;
    }
  }
}

/**
return true if up was pressed, and reset up.
*/
bool isUpPressed() {
  if (_upPressed) {
    _hasInputBeenRead = true; //used to keep a button from repeating when held down.
    _upPressed = false;
    return true;
  }
  return false;
}

/**
return true if down was pressed, and reset down.
*/
bool isDownPressed() {
  if (_downPressed) {
    _hasInputBeenRead = true; //used to keep a button from repeating when held down.
    _downPressed = false;
    return true;
  }
  return false;
}

/**
return true if left was pressed, and reset left.
*/
bool isLeftPressed() {
  if (_leftPressed) {
    _hasInputBeenRead = true; //used to keep a button from repeating when held down.
    _leftPressed = false;
    return true;
  }
  return false;
}

/**
return true if right was pressed, and reset left.
*/
bool isRightPressed() {
  if (_rightPressed) {
    _hasInputBeenRead = true; //used to keep a button from repeating when held down.
    _rightPressed = false;
    return true;
  }
  return false;
}

/**
return true if select was pressed and reset select.
*/
bool isSelectPressed() {
  if (_selectPressed) {
    _hasInputBeenRead = true; //used to keep a button from repeating when held down.
    _selectPressed = false;
    return true;
  }
  return false;
}

/**
return true if any button is pressed, else return false.
*/
bool isAnyButtonPressed() {
  return ((isSelectPressed()) || (isLeftPressed()) || (isRightPressed()) || (isUpPressed()) || (isDownPressed()));
}

//the file navigator
int _numFilesInDir = 0;
int _currentNavigatedFileCount = 0;
String _fileToOpen;
String _tempFileName;

/**
get the file to open
*/
// String getFileToOpen() {
//   return _fileToOpen;
// }

/**
open the directory
*/
File openDirectory(String address) {
  File myFile = SD.open(address);
  String message;
  if (myFile) {
    message = "directory opened";
    DEBUG_LN(message);
    // resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
  }
  else {
    message = "err opening dir";
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
  }
  return myFile;
}

/**
close the file
*/
void closeFile(File root) {
  root.close();
  String message = "closing file";
    DEBUG_LN(message);
    // resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
}

/**
display files within the directory. then waits for button presses to respond.
up or down will navigate, right will open a file if it's valid, select will 
go back to the config.
*/
void displayFiles(String address) {
  bool loopDisplayFilesCondition = true; //if to continue displaying files.
  File root;
  File entry;
  while (loopDisplayFilesCondition) {
    root = openDirectory(address);//open the directory.
    int fileCount = 0;//file count is the count of displayed files.
    int row = 0;//row is the row displayed on the lcd screen.
    lcd->clear(); //clear the lcd
    while (row < LCD_ROWS) {//only loop if not exceeded rows to display.
      entry = nextFile(root);//opens the next file.
      fileCount ++;//iterates the count of displayed files.
      //checks if the current file is high enough in the navigated file count to 
      //display on the lcd screen.
      if (fileCount > _currentNavigatedFileCount) {
        if (isFile(entry)) {//if file is open.
            String fileNameThisRow = getFileName(entry);
          if (row == 0) {//if this is the first file in the row, set the temporary file name.
            _tempFileName = getFileName(entry);
            fileNameThisRow += "_";
          }
          displayMsgAtRow(fileNameThisRow, row);//display file on lcd
          row ++;//iterate row.
        } else {//file could not be opened so end loop.
          row = LCD_ROWS;//match loop condition to end loop.
        }
      }
      closeFile(entry);
    }
    lcd->display();//display lcd screen.
    //loop to check button presses, return value is outer loop condition.
    loopDisplayFilesCondition = checkButtonPressesInDisplayFiles();
    closeFile(root);//close directory so it can be opened and files freshly iterated again.
  }
}

/**
loop until a button has been pressed, this also controls the loop
it exists inside, in case a file is loaded, or select is pressed.
*/
bool checkButtonPressesInDisplayFiles() {
  bool loopButtonCheckingCondition = true;
  while (loopButtonCheckingCondition) {
    readButtons();
    if (isDownPressed()) {
      _currentNavigatedFileCount ++;
      // break;
      loopButtonCheckingCondition = false;
    }
    else if (isUpPressed()) {
      _currentNavigatedFileCount --;
      if (_currentNavigatedFileCount < 0) {
        _currentNavigatedFileCount = 0;
      }
      // break;
      loopButtonCheckingCondition = false;
    }
    else if (isRightPressed()) {
      if (isFileNamevalid()) {
        setFile();
        loopButtonCheckingCondition = false;
        stateInt = 1; //set the state to navigate a row.
        String message = "name valid";
        //display lcd message to display the name is valid.
        // resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
        return false;
      }
    }
    else if (isSelectPressed()) {
      stateInt = 10;//set state int to config.
      loopButtonCheckingCondition = false;
    }
  }
  return true;
}

/**
check if the file name is valid, it must end in .bmp
*/
bool isFileNamevalid() {
  String lowerCaseFileName = toLowerCase(_tempFileName);
  if (lowerCaseFileName.endsWith(".bmp")) {
    return true;
  }
  return false;
}

/**
convert a string to lower case
*/
String toLowerCase(String myString) {
  for (int i = 0; i < myString.length(); i++) {
    myString[i] = tolower(myString[i]);
  }
  return myString;
}

/**
set the file to open
*/
void setFile() {
  _fileToOpen = _tempFileName;
}

/**
next file
*/
File nextFile(File root) {
  return root.openNextFile();
}

/**
return true if there is a file opened, else return false.
*/
bool isFile(File entry) {
    if (!entry) {
      return false;
    }
    return true;
}

// /**
// close the file
// */
// void closeFile() {
//   _entry.close();
// }

/**
return the string of the name of the opened file
*/
String getFileName(File entry) {
  String fileName = entry.name();
  return fileName;
}


/**
file uses file navigator at root.
*/
void navigateFilesAtRoot() {
  displayFiles("/");
}


//LED strip handler
Adafruit_NeoPixel * strip;

/**
strip creation function
*/
void createStrip() {
  //instantiate the strip
  strip = new Adafruit_NeoPixel(ledCount, LED_PIN, NEO_GRB + NEO_KHZ800);
  strip->begin(); //initialize NeoPixel strip object (REQUIRED)
  strip->setBrightness(brightness); //set the brightness.
  strip->show(); //turn off all pixels.
}
// /**
// the constructor, constructs the strip object
// */
// LblLedStripHandler() : strip(ledCount, LED_PIN, NEO_GRB + NEO_KHZ800) {
//   DEBUG_LN("LblLedStripHandler constructor called.");
//   strip.begin(); //initialize NeoPixel strip object (REQUIRED)
//   strip.setBrightness(brightness); //set the brightness.
//   strip.show(); //turn off all pixels.
// }


/**
set the brightness
*/
void setLedBrightness() {
  strip->setBrightness(brightness);
  strip->show();
}

/**
set a pixel to true or false at a specific location
*/
void setPixel(int pixelIndex, bool isTrue) {
  if (isTrue) {
    strip->setPixelColor(pixelIndex, strip->Color(0,0,255));
    DEBUG_MSG("set pixel to true at index ");
    DEBUG_MSG(pixelIndex);
  }
  else {
    strip->setPixelColor(pixelIndex, strip->Color(0,0,0));
    DEBUG_MSG("set pixel to false at index ");
    DEBUG_MSG(pixelIndex);
  }
}



/**
bitmap handler is for opening the bitmap on the SD card, and decoding it. it nees to open
and verify a file before reading pixels as true or false.
*/
bool fileOk = false; //if file is ok to use
File bmpFile; //the file itself
int _bytesPerRow;
int _numEmptyBytesPerRow;


int currentRow; //current row of bitmap being displayed.
// BMP header fields
uint16_t headerField;
// uint32_t fileSize;
uint32_t imageOffset;
// DIB header
// uint32_t headerSize;
uint32_t imageWidth;
uint32_t imageHeight;
uint16_t colourPlanes;
uint16_t bitsPerPixel;
uint32_t compression;
// uint32_t imageSize;
// uint32_t xPixelsPerMeter;
// uint32_t yPixelsPerMeter;
// uint32_t totalColors;
// uint32_t importantColors;


void incrementRow() {
  currentRow++;
  if (currentRow >= imageHeight) {
    currentRow = 0;
  }
  isLightOnAtColumn(currentRow); //set the lights array according to the current row.
}

void decrementRow() {
  currentRow--;
  if (currentRow < 0) {
    currentRow = imageHeight - 1;
  }
  isLightOnAtColumn(currentRow); //set the lights array according to the current row.
}

/**
*read a byte and return it as an unsigned 8 bit int.
* code was sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
uint8_t read8Bit(){
  if (!bmpFile) {
    return 0;
  }
  else {
    return bmpFile.read();
  }
}

/**
*read 2 bytes, and return them as an unsigned 16 bit int.
* code sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
uint16_t read16Bit(){
  uint16_t lsb, msb;
  if (!bmpFile) {
    return 0;
  }
  else {
    lsb = bmpFile.read();
    msb = bmpFile.read();
    return (msb<<8) + lsb;
  }
}

/**
*read 4 bytes, an return them as an unsigned 32 bit int.
* code was sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
uint32_t read32Bit(){
  uint32_t lsb, b2, b3, msb;
  if (!bmpFile) {
    return 0;
  }
  else {
    lsb = bmpFile.read();
    b2 = bmpFile.read();
    b3 = bmpFile.read();
    msb = bmpFile.read();
    return (msb<<24) + (b3<<16) + (b2<<8) + lsb;
  }
}


/**
*verify the opened file.
* code modified after being sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
bool verifyFile() {
  if (!bmpFile) {
    String message;
    message = "unable to open file";
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    fileOk = false;
    return false;
  }
  if (!readFileHeaders()){
    String message;
    message = "Unable to read file headers";
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    fileOk = false;
    return false;
  }
  if (!checkFileHeaders()){
    String message;
    message = "Not compatable file";
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    fileOk = false;
    return false;
  }
  // //image width check, uncomment this later.
  if (ledCount < imageWidth) {
    String message;
    message = "Image width greater then LED count ";
    message += ledCount;
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    fileOk = false;
    return false;
  }
  // all OK
  String message;
  message = "file OK";
  DEBUG_LN("BMP file all OK");
  resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
  fileOk = true;
  return true;
}

/**
*read file headers from file and set instance variables.
* code modified after being sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
bool readFileHeaders(){
  if (bmpFile) {
    // reset to start of file
    bmpFile.seek(0);
    
    // BMP Header
    headerField = read16Bit();
    // fileSize = read32Bit();
    // read16Bit(); // reserved
    // read16Bit(); // reserved
    bmpFile.seek(10);
    imageOffset = read32Bit();

    // DIB Header
    // headerSize = read32Bit();
    bmpFile.seek(18);
    imageWidth = read32Bit();
    imageHeight = read32Bit();
    colourPlanes = read16Bit();
    bitsPerPixel = read16Bit();
    compression = read32Bit();
    // imageSize = read32Bit();
    // xPixelsPerMeter = read32Bit();
    // yPixelsPerMeter = read32Bit();
    // totalColors = read32Bit();
    // importantColors = read32Bit();
    //the empty bytes in a row, becase a row must be a multiple of 4 bytes.;
    _numEmptyBytesPerRow = ((4 - ((3 * imageWidth) % 4)) % 4); 
    //the number of bytes in a row.
    _bytesPerRow = (3 * imageWidth) + _numEmptyBytesPerRow;
    return true;
  }
  else {
    return false;
  }
}

/**
*check file header values are valid.
* code was sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
bool checkFileHeaders(){

  // BMP file id
  // if (headerField != 0x4D42){
  //   String message = "file is not Windows 3.1x, 95, NT, ... etc. bitmap file id.";
  //   DEBUG_LN(message);
  //   errorMessage(message);
  //   return false;
  // }
  // must be single colour plane
  // if (colourPlanes != 1){
  //   String message = "file is not single colour plane";
  //   DEBUG_LN(message);
  //   errorMessage(message);
  //   return false;
  // }
  // only working with 24 bit bitmaps
  if (bitsPerPixel != 24){
    String message = "is not 24 bit bitmap.";
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    return false;
  }
  // no compression
  if (compression != 0){
    String message = "bitmap is compressed.";
    DEBUG_LN(message);
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    return false;
  }
  // all ok
  return true;
}

/**
*print file hader values to serial.
* code sourced from: 
*https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*/
void serialPrintHeaders() {
  DEBUG_MSG("imageOffset : ");
  DEBUG_LN(imageOffset);
  DEBUG_MSG("imageWidth : ");
  DEBUG_LN(imageWidth);
  DEBUG_MSG("imageHeight : ");
  DEBUG_LN(imageHeight);
  DEBUG_MSG("colourPlanes : ");
  DEBUG_LN(colourPlanes);
  DEBUG_MSG("bitsPerPixel : ");
  DEBUG_LN(bitsPerPixel);
  DEBUG_MSG("compression : ");
  DEBUG_LN(compression);
}


/**
*open the file, print an error message if it is not opened. return true if the file is opened, otherwise false.
*/
bool openFile(String fileName) {
bmpFile = SD.open(fileName, FILE_READ);
String message;
if (!bmpFile) {
    message = "Unable to open file"
    DEBUG_MSG(message);
    fileOk = false;
    resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
    return false;
  }
  message = "file opened";
  resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
  return true;
}


/**
return true if light is on at column, first it gets the pixel row offset, based on the base pixel row offset
and adding the empty filler bytes (because the bytes containing pixel info must be multiples of 4) and also
the column times 3 for for each pixel in that row. it then sets the read position for the SD reader, then reads
a buffer of 3 bytes. It then passes the read bytes to isPixelTrue() and gets back if the pixel is 
true or not, returning that.
*/
bool isLightOnAtColumn(int column) {
  if ((column < ledOffset) || (column >= (ledOffset + imageWidth))) {
    return false;
  }
  uint8_t pixelBuffer[NUM_BYTES_PER_PIXEL]; //create the pixel buffer array which is used to read a pixel.
  //find the pixel row offset for this specific pixel.
  int pixelRowFileOffset = imageOffset + ((column - ledOffset) * NUM_BYTES_PER_PIXEL) + ((imageHeight - currentRow - 1) * _bytesPerRow);
  bmpFile.seek(pixelRowFileOffset);//seek to the pixel row offset.
  bmpFile.read(pixelBuffer, NUM_BYTES_PER_PIXEL);//read into the pixel buffer.
  return isPixelTrue(pixelBuffer[0], pixelBuffer[1], pixelBuffer[2]);//return if the pixel is true or not.
}

/**
*this find the initial binary shift which additionally shifts the 
* binary values of the first byte in the lights array.
*/
int calculateInitialBinaryShift(int imageWidth) {
    int initialBinaryShift = 8 - imageWidth;
  if (initialBinaryShift < 0) {
    initialBinaryShift = 0;
  }
  return initialBinaryShift;
}

/**
*is passed 3 colors, checks the combined saturation and returns if the pixel is true or false.
*/
bool isPixelTrue(uint8_t blue, uint8_t red, uint8_t green) {
  uint8_t total = blue + red + green;
  int redInt = (int)red;
  int blueInt = (int)blue;
  int greenInt = (int)green;
  int totalInt = redInt + blueInt + greenInt;
  if (totalInt < 384) {
    return true;
  }
  return false;
}


/**
return true if bit in byte is true at bit position, 
note: position goes from left to right.
*/
bool isBitTrueInByte(byte myByte, int bitPos) {
  bitPos = 7 - bitPos;
  bool isBitTrue = ((myByte >> bitPos) & 0X01);
  return isBitTrue;
}


/**
*initialize the SD card.
*/
void initializeCard() {
  DEBUG_MSG("beginning initialization of SD card");

  if (!SD.begin(CHIP_SELECT)) {
    String message = "SD initialization failed";
    DEBUG_LN(message);
    resetAndDisplayStringLcd(message);
    while (1); //infinate loop to force resetting arduino
  }
  DEBUG_LN("SD Initalized successfully"); //if you reach here setup successfully
  DEBUG_LN("----------------------------\n");
  String message = "SD initialized successfully";
  resetAndDisplayStringLcd(message);
  displayStringLcd(message);
  delay(400);
}

/**
print a different character if the value is true or false.
*/
void printBool(bool boolToPrint) {
  if (boolToPrint) {
    DEBUG_WR(1);
  }
  else {
    DEBUG_MSG(" ");
  }
}


/**
delte and recreate the LED strip handler with a strop object, likely because the LED count has changed.
*/
void recreateLedStripHandler() {
  delete strip;
  createStrip();
}

void showLightsForRow() {
  for (int i=0; i<ledCount; i++) {
    //set the pixel at i, if it is true in lights array at i.
    setPixel(i, isLightOnAtColumn(i));
  }
  strip->show();
}

/**
increase the led count, if its above max, set to 0.
*/
void increaseLedCount() {
  ledCount ++;
  if (ledCount > LED_COUNT) {
    ledCount = 1;
  }
  checkLedOffset();
}

/**
decrease led count, if its below min, set to max.
*/
void decreaseLedCount() {
  ledCount --;
  if (ledCount < 1) {
    ledCount = LED_COUNT;
  } else {
    
    //set the pixel of the led strip that has been removed to false.
    setPixel(ledCount, false); 
    strip->show();
  }
  checkLedOffset();
}

/**
increasse the led offset, if it's above max, set it to 0.
*/
void increaseLedOffset() {
  ledOffset ++;
  if (ledOffset > (ledCount - imageWidth)) {
    ledOffset = 0;
  }
  showLightsForRow();
}

/**
decrease the LED offset, if it's below 0, set it to the max.
*/
void decreaseLedOffset() {
  ledOffset --;
  if (ledOffset < 0) {
    ledOffset = (ledCount - imageWidth);
    EEPROM.put(EEPROM_OFFSET, ledOffset);
  }
  showLightsForRow();
}

/**
increase the brightness by a fixed amount, if it is over the max, set it to 0.
*/
void increaseBrightnessVar() {
  brightness += BRIGHTNESS_INCREMENT;
  if (brightness > 255) {
    brightness = 0;
  }
  setLedBrightness();
}

/**
decrease brightness by a fixed amount, if it is under the min, set it to 255.
*/
void decreaseBrightnessVar() {
  brightness -= BRIGHTNESS_INCREMENT;
  if (brightness <= 0) {
    brightness = 255;
  }
  setLedBrightness();
}

/**
reads all the eeprom configure data, setting global variables.
*/
void readAllEepromData() {
  readEepromRow();
  readEepromLedCount();
  readEepromBrightness();
  readEepromLedOffset();
}

/**
writes all the eeprom configure data, writing from global variables.
passing the functions the global variables.
*/
void writeAllEepromData() {
  writeEepromLedCount(ledCount);
  writeEepromBrightness(brightness);
  writeEepromLedOffset(ledOffset);
}

/**
read the led count from the EEPROM. if out of bounds, set to max bounds.
*/
int readEepromLedCount() {
  EEPROM.get(EEPROM_LED_COUNT, ledCount);
  if ((ledCount < 1) || (ledCount > LED_COUNT)) {
    ledCount = LED_COUNT;
    EEPROM.put(EEPROM_LED_COUNT, ledCount);
  }
  return ledCount;
}

/**
write the led count to the EEPROM. if out of bounds, set it to max.
*/
void writeEepromLedCount(int ledCount) {
  if ((ledCount < 1) || (ledCount > LED_COUNT)) {
    ledCount = LED_COUNT;
  }
  EEPROM.put(EEPROM_LED_COUNT, ledCount);
}



/**
check that the led offset is within bounds
*/
void checkLedOffset() {
  if (ledOffset > (ledCount - imageWidth)) {
    ledOffset = ledCount - imageWidth;
  } else 
  if (ledOffset < 0) {
    ledOffset = 0;
  }
}

/**
read the image offset from the EEPROM setting the global variable ledOffset
in the process and returing that value, make sure its within bounds.
*/
int readEepromLedOffset() {
  EEPROM.get(EEPROM_OFFSET, ledOffset);
  checkLedOffset();
  return ledOffset;
}

/**
set the ledOffset global var to offset, make sure its within bounds, then 
write it to  the EEPROM.
*/
void writeEepromLedOffset(int offset) {
  ledOffset = offset;
  checkLedOffset();
  EEPROM.put(EEPROM_OFFSET, ledOffset);
}

/**
check the unsigned int is within bounds.
*/
uint8_t checkWithinUint8Bounds(uint8_t value) {
  if (value <0) {
      value = 0;
  } else if (value > 255){
    value = 255;
  }
  return value;
}

/**
read the brigthness form the EEPROM. if it is outside the bounds, reset it to 25.
*/
uint8_t readEepromBrightness() {
  EEPROM.get(EEPROM_BRIGHTNESS, brightness);
  brightness = checkWithinUint8Bounds(brightness);
  EEPROM.put(EEPROM_BRIGHTNESS, brightness);
  return brightness;
}

/**
make sure brigthness is within bounds, then write the brightness to the EEPROM
*/
void writeEepromBrightness(uint8_t writeBrightness) {
  brightness = writeBrightness;
  brightness = checkWithinUint8Bounds(brightness);
  EEPROM.put(EEPROM_BRIGHTNESS, brightness);
}

/**
read the row number from the EEPROM. If it is outside the image bounds, reset it to 0.
*/
int readEepromRow() {
  // int rowRead = 0;
  EEPROM.get(EEPROM_ROW, currentRow);
  if ((currentRow <0) | (currentRow >= imageHeight)) {
    currentRow = 0;
  }
  return currentRow;
}

/**
write the row number to the EEPROM, first check if it's within the image bounds.
*/
void writeEepromRow(int row) {
  if ((row <=0) | (row >= imageHeight)) {
    row = 0;
  }
  EEPROM.put(EEPROM_ROW, row);
}


/**
gets the byte at index number in the byte.
*/
bool getBitFromByte(byte myByte, uint8_t index) {
  return (myByte >> (index)) & 0x01;
}

/**
set bit at location in byte
*/
byte setBitInByte(byte myByte, uint8_t index, bool myBit) {
  if (myBit) {
    myByte |= (0x01 << (index));
  }
  else {
    myByte &= ~(0x01 << (index));
  }
  return myByte;
}

/**
get the value of bits of the bitCount number of bits from 
the byte
*/
uint8_t getBitsFromByte(byte myByte, uint8_t bitCount) {
  uint8_t apersandCompare = 1;
  for (int i = 0; i< bitCount - 1; i++) {
    apersandCompare = (apersandCompare << 1) + 1;
  }
  return apersandCompare & myByte;
}

#define NUM_BITS_STATE 5
/**
get the state from the stateInt
*/
uint8_t getState() {
  return getBitsFromByte(stateInt, NUM_BITS_STATE);
}

/**
set weither in row mode or config mode
*/
void setUIInRow(bool inRow) {
  stateInt = setBitInByte(stateInt, 7, inRow);
}

/**
get if in row or config mode
*/
bool isUIInRow() {
  return (getBitFromByte(stateInt, 7));
}

/**
this is when the ui state is leaving config and going into row.
first it writes all eeprom data,
then runs verifyFile() which is mainly to check image width,
 then sets the stateInt to the row state int.
*/
void setStateDepartingConfig() {
  writeAllEepromData();
  verifyFile();
  stateInt = 1;
}

/**
the intro menu screen.
*/
void uiIntro() {
  if (stateInt != 0) {
    return;
  }
  String message;
  message += String(imageWidth);
  message += "x";
  message += String(imageHeight);
  resetAndDisplayStringLcd(message);
  while(1) {
    displayStringLcd(message);
    readButtons();
    if ((isUpPressed()) || (isDownPressed())) {
      currentRow = readEepromRow(); //read the current row from the EEPROM.
      stateInt = 10;
      break;
    }
    if (isSelectPressed()) {
      stateInt = 10;//enter config mode.
      break;
    }
  }
}

/**
the display row ui function
*/
void uiDisplayRow() {
  if (stateInt != 1) {
    return;
  }
  String message;
  message = "current row: ";
  message += (currentRow + 1);
  resetAndDisplayStringLcd(message);
  showLightsForRow();
  while (1) {
    displayStringLcd(message);
    readButtons();
    if (isUpPressed()) {
      decrementRow();
      break;
    } else
    if (isDownPressed()) {
      incrementRow();
      break;
    }
    if (isRightPressed()) {
      uiSaveRowEeprom(currentRow);
      return;
    }
    if (isLeftPressed()) {
      uiLoadRowEeprom();
      return;
    }
    if (isSelectPressed()) {
      stateInt = 10;//enter config mode.
      break;
    }
  }
}

/**
ui to navigate files
*/
void uiNavigateFiles() {
  if (stateInt != 20) {
    return;
  }
  navigateFilesAtRoot();
}

/**
ui to reset brightness, offset, and LED count
*/
void uiReset() {
  if (stateInt != 13) {
    return;
  }
  String message;
  message = "Reset";
  resetAndDisplayStringLcd(message);
  while(1) {
    displayStringLcd(message);
    readButtons();
    if (isUpPressed()) {
      stateInt = 12;
      break;
    }
    if (isDownPressed()) {
      stateInt = 10;
      break;
    }
    if (isRightPressed()) {
      brightness = 1;
      ledCount = 60;
      ledOffset = 0;
      message = "Resetting...";
      resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
      setLedBrightness();
      break;
    } else
    if (isSelectPressed()) {
      // writeAllEepromData(); 
      // stateInt = 1;
      setStateDepartingConfig();
      break;
    }
  }
}

/**
the UI for the brigtness in the menu, up goes to the last menu setting, down to the next
left decreases the brightness, right increases the brightness, select goes to row mode.
*/
void uiBrightness() {
  if (stateInt != 10) {
    return;
  }
  String message;
  message = "brightness: ";
  message += String(brightness);
  resetAndDisplayStringLcd(message);
  while(1){
    displayStringLcd(message);
    readButtons();
    if (isRightPressed()) {
      increaseBrightnessVar();
      break;
    } else
    if (isLeftPressed()) {
      decreaseBrightnessVar();
      break;
    } else
    if (isUpPressed()) {
      stateInt = 13;
      break;
    } else
    if (isDownPressed()) {
      stateInt = 11; //enter offset config mode.
      break;
    } else
    if (isSelectPressed()) {
      // writeAllEepromData(); 
      // stateInt = 1;
      setStateDepartingConfig();
      break;
    }
  }
}

/**
the led offset setting, display current offset, up goes to previous config, down to next, 
left decreases offset, right increases offset, select goes back to row mode.
*/
void uiOffset() {
  if (stateInt != 11) { //in offset config mode.
    return;
  }
  String message;
  message = "offset: ";
  message += ledOffset;
  resetAndDisplayStringLcd(message);
  showLightsForRow();
  while(1) {
    displayStringLcd(message);
    readButtons();
    if (isUpPressed()) {
      stateInt = 10;
      break;
    } else
    if (isDownPressed()) {
      stateInt = 12;
      break;
    } else
    if (isLeftPressed()) {
      decreaseLedOffset();
      break;
    } else
    if (isRightPressed()) {
      increaseLedOffset();
      break;
    } else
    if (isSelectPressed()) {
      // writeAllEepromData();
      // stateInt = 1;
      setStateDepartingConfig();
      break;
    }
  }
}

/**
the ui configure for the led count.
*/
void uiLedCount() {
  if (stateInt != 12) { //led count config mode.
    return;
  }
  String message;
  message += "LED count: ";
  message += (String)ledCount;
  resetAndDisplayStringLcd(message);
  while(1) {
    displayStringLcd(message);
    readButtons();
    if (isUpPressed()) {
      stateInt = 11;
      break;
    } else
    if (isDownPressed()) {
      stateInt = 13;
      break;
    } else
    if (isLeftPressed()) {
      decreaseLedCount();
      recreateLedStripHandler();
      showLightsForRow();
      break;
    } else
    if (isRightPressed()) {
      increaseLedCount();
      recreateLedStripHandler();
      showLightsForRow();
      break;
    } else
    if (isSelectPressed()) {
      // writeAllEepromData();
      // stateInt = 1;
      setStateDepartingConfig();
      break;
    }
  }
}

/**
display message, and load eeprom row to bitmap handler current row
*/
void uiLoadRowEeprom() {
  currentRow = readEepromRow();
  String message = "loading row";
  resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
  stateInt = 1;
}

/**
display message, and save current row to eeprom
*/
void uiSaveRowEeprom(int row) {
  String message;
  message += "saving row: ";
  message += String(row + 1);
  resetAndDisplayMessageWithBreakableLoopLcd(message, LCD_SHORT_MESSAGE_DURATION);
  stateInt = 1;
}

/**
*creates bitmap handler object, then opens bitmap file, reads the headers, then loops each row in the bitmap
*and decodes each row, printing the binary values based on saturation.
*/
void setup() {
  //set serial to dispaly on ide. This cannot be used when using the Neopixel Adafruit light strip
  //library, or it interferes with the light strip.
  DEBUG_BEGIN;
  //read all eerpom data
  readAllEepromData();
  //create lcd
  lcd = new LiquidCrystal(rs, en, d4, d5, d6, d7);
  lcd->begin(LCD_COLS, LCD_ROWS);
  delay(1000);
  initializeCard();
  navigateFilesAtRoot();
  
  String lowerCaseName = toLowerCase(_fileToOpen);
  openFile(lowerCaseName);
  // //verify file, this includes reading the headers which is necessary to decode the bitmap.
  verifyFile();
  //instantiate light strip handler
  createStrip();
  stateInt = 0;
  
  uiIntro();
  while(stateInt!=1) {

    uiBrightness();
    uiOffset();
    uiLedCount();
    // showLightsForRow();
    uiReset();
    //if the file is not ok possibly because the image width is greater than the LED count.
    //stay in the config loop.
    if ((!fileOk) && (stateInt == 1)) {
      resetAndDisplayStringLcd("file not ok");
      stateInt = 0;
      uiIntro();
    }
  }
  
  
}
void loop() {
  //   return;
  // }
  // uiIntro();
  uiDisplayRow();
  // uiBrightness();
  // uiOffset();
  // uiLedCount();
  // // showLightsForRow();
  // uiReset();
}
