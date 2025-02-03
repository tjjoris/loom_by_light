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
*some of the code was modified after being sourced from bitsnbytes.co.uk:
https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*@author Luke Johnson
*@date 2025-February-03
*/

#include <SPI.h>
#include <SD.h>
#include <EEPROM.h>


#define CHIP_SELECT 10 //the chip select used for the SD card.
#define LED_COUNT 60 //the number of lights on the light strip.
#define LED_PIN 1 //the pin the data line for the addressable LED strip is connected to.
// NeoPixel brightness, 0 (min) to 255 (max)
#define LCD_ROWS 2 //the number of character rows on the lcd screen, this is how many lines fit on the lcd screen.
#define LCD_COLS 16 //the number of character columns on the lcd screen, this is how many characters fit on one line.
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //the pin values for the lcd display.
#define BRIGHTNESS_INCREMENT 26

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
#define DEBUG_WR(x) Serial.write(x);
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
String _storedMessage = "";    //the stored message to be written to the lcd screen.
uint8_t _updateCounter = 0; //the counter to determine if the substring should be continued.
uint8_t _updateCounterMax = 25; //the max the counter should go for the substring to be continued.
uint8_t _charCount = 0; //the character count in the message string.

/**
a simple function for displaying a message
*/
void displaySimpleMsg(String message) {
  lcd->clear();
  lcd->setCursor(0,0);
  lcd->print(message);
  lcd->display();
}

/**
clear lcd
*/
void clearLcd() {
  lcd->clear();
}

/**
display lcd
*/
void displayLcd() {
  lcd->display();
}

/**
display a message at row.
*/
void displayMsgAtRow(String message, int row) {
  lcd->setCursor(0,row);
  lcd->print(message);
}


/**
set the stored lcd message, this is what update uses to know if it needs to update the screen.
*/
void storeMessage(String message) {
  _storedMessage = message; //the stored message to show on the lcd screen.
  _updateCounter = 0; //this it the counter to know when to update the lcd screen.
  _charCount = 0; //this is the character count, used to write the message to the lcd screen char by char.
}

/**
loop through each row and column, and write to the lcd screen
the character written is known by the _messageBeingDisplayed and
charCount determins where in that message to print the char.
*/
void lcdWrite() {
  //loop for each row in the lcd screen.
    for (int row = 0; row < LCD_ROWS; row ++) {
      //set the cursor to the correct row.
      lcd->setCursor(0, row);
      for (int col = 0; col < LCD_COLS; col ++) {
        if (_storedMessage.length() > _charCount) {
          lcd->write(_storedMessage[_charCount]);
        }
        if (_charCount < _storedMessage.length()) {
          _charCount ++;
        }          
      }
    }
}

/**
This function is called when finished writing to lcd rows and columns,
resets _charCount if _charCount is greater than the message length.
*/
void resetCharCount() {
    if (_charCount >= _storedMessage.length()) {
      _charCount = 0;
    }
}

/**
increments the update counter if is lower than the max. 
Otherwise reset it to 0.
*/
void incrementUpdateCounter() {
  if (_updateCounter < _updateCounterMax) {
    _updateCounter ++;
    return;
  }
  //the update counter has reached the max, reset it to 0 and end the function.
  if (_updateCounter >= _updateCounterMax) {
    _updateCounter = 0;
  }

}

/**
this function iterates through _updateCounter, and when it is at 0, it calls lcdWrite(),
when it does this, it checks if _charCount is at the max message length, if it it is, resets
_charCount.
when _updateCounter is at max, it is reset. 
*/
void update() {
  //the update counter is at 0, therefore update the lcd screen.
  if (_updateCounter <= 0) {
    lcd->clear();
    lcdWrite();
    resetCharCount();
    lcd->display();
  }
  incrementUpdateCounter();
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
String getFileToOpen() {
  return _fileToOpen;
}

/**
open the directory
*/
File openDirectory(String address) {
  File myFile = SD.open(address);
  String message;
  if (myFile) {
    message = "directory opened"
    DEBUG_LN(message);
    // displaySimpleMsg("opening dir");
    // delay(1000);
  }
  else {
    message = "failed directory open";
    DEBUG_LN(message);
    displaySimpleMsg("err opening dir");
    delay(3000);
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
    // displaySimpleMsg(message);
    // delay(1000);
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
    clearLcd(); //clear the lcd
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
    displayLcd();//display lcd screen.
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
        displaySimpleMsg("name valid");
        delay(1500);
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
  showStrip();
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
display the set lights on the light strip.
*/
void showStrip() {
  strip->show();
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


// /**
// *constructor, sets instance variables for filename.
// */
// BitmapHandler(String filename){
//   fileOk = false;
//   bmpFilename = filename;
//   currentRow = 0;
// }


bool isFileOk() {
  return fileOk;
}

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
print an error message
*/
void errorMessage(String message) {
    storeMessage(message);
    for (int i = 0; i< 10; i++) {
      update();
      readButtons();
      delay(100);
      if (isAnyButtonPressed()) {
        break;
      }
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
    errorMessage(message);
    fileOk = false;
    return false;
  }
  if (!readFileHeaders()){
    String message;
    message = "Unable to read file headers";
    DEBUG_LN(message);
    errorMessage(message);
    fileOk = false;
    return false;
  }
  if (!checkFileHeaders()){
    String message;
    message = "Not compatable file";
    DEBUG_LN(message);
    errorMessage(message);
    fileOk = false;
    return false;
  }
  // //image width check, uncomment this later.
  if (ledCount < imageWidth) {
    String message;
    message = "Image width greater then LED count ";
    message += ledCount;
    DEBUG_LN(message);
    errorMessage(message);
    // fileOk = false;
    // return false;
  }
  // all OK
  String message;
  message = "file OK";
  DEBUG_LN("BMP file all OK");
  errorMessage(message);
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
    errorMessage(message);
    return false;
  }
  // no compression
  if (compression != 0){
    String message = "bitmap is compressed.";
    DEBUG_LN(message);
    errorMessage(message);
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
  DEBUG_MSG("filename : ");
  DEBUG_LN(bmpFilename);
  // BMP Header
  // DEBUG_MSG(F("headerField : "));
  // Serial.println(headerField, HEX);
  DEBUG_MSG("fileSize : ");
  DEBUG_LN(fileSize);
  DEBUG_MSG("imageOffset : ");
  DEBUG_LN(imageOffset);
  DEBUG_MSG("headerSize : ");
  DEBUG_LN(headerSize);
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
  DEBUG_MSG("imageSize : ");
  DEBUG_LN(imageSize);
  DEBUG_MSG("xPixelsPerMeter : ");
  DEBUG_LN(xPixelsPerMeter);
  DEBUG_MSG("yPixelsPerMeter : ");
  DEBUG_LN(yPixelsPerMeter);
  DEBUG_MSG("totalColors : ");
  DEBUG_LN(totalColors);
  DEBUG_MSG("importantColors : ");
  DEBUG_LN(importantColors);
}


/**
*open the file, print an error message if it is not opened. return true if the file is opened, otherwise false.
*/
bool openFile(String fileName) {
bmpFile = SD.open(fileName, FILE_READ);
if (!bmpFile) {
    DEBUG_MSG(F("BitmapHandler : Unable to open file "));
    DEBUG_LN(filename);
    fileOk = false;
    errorMessage("unable to open");
    return false;
  }
  errorMessage("file opened");
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
    storeMessage(message);
    while (1); //infinate loop to force resetting arduino
  }
  DEBUG_LN("SD Initalized successfully"); //if you reach here setup successfully
  DEBUG_LN("----------------------------\n");
  storeMessage("SD initialized successfully");
  update();
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
    showStrip();
  }
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
    showStrip(); //show the strip again to update the removed pixel.
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
  int rowRead = 0;
  EEPROM.get(EEPROM_ROW, rowRead);
  if ((rowRead <=0) | (rowRead >= imageHeight)) {
    rowRead = 0;
    EEPROM.put(EEPROM_ROW, rowRead);
  }
  return rowRead;
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
  // message = "bmp width: ";
  message += String(imageWidth);
  message += "x";
  message += String(imageHeight);
  storeMessage(message);
  update();
  readButtons();
  if ((isUpPressed()) || (isDownPressed())) {
    currentRow = readEepromRow(); //read the current row from the EEPROM.
    stateInt = 1;
  }
  if (isSelectPressed()) {
    stateInt = 10;//enter config mode.
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
  storeMessage(message);
  update();
  showLightsForRow();
  readButtons();
  if (isUpPressed()) {
    decrementRow();
  } else
  if (isDownPressed()) {
    incrementRow();
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
  }
}

// /**
// the ui for showing the screen to navigate files
// */
// void uiLoadOption() {
//   if (stateInt != 13) {
//     return;
//   }
//   String message;
//   message = "Load";
//   storeMessage(message);
//   update();
//   readButtons();
//   if (isUpPressed()) {
//     stateInt = 12;
//   }
//   else if (isDownPressed()) {
//     stateInt = 10;
//   }
//   else if (isRightPressed()) {
//     stateInt = 20;
//   }
// }

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
  storeMessage(message);
  update();
  readButtons();
  if (isUpPressed()) {
    stateInt = 12;
  }
  if (isDownPressed()) {
    stateInt = 10;
  }
  if (isRightPressed()) {
    brightness = 1;
    ledCount = 60;
    ledOffset = 0;
    message = "Resetting...";
    storeMessage(message);
    update();
    delay(2500);
    setLedBrightness();
  } else
  if (isSelectPressed()) {
    // writeAllEepromData(); 
    // stateInt = 1;
    setStateDepartingConfig();
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
  storeMessage(message);
  update();
  readButtons();
  if (isRightPressed()) {
    increaseBrightnessVar();
  } else
  if (isLeftPressed()) {
    decreaseBrightnessVar();
  } else
  if (isUpPressed()) {
    stateInt = 13;
  } else
  if (isDownPressed()) {
    stateInt = 11; //enter offset config mode.
  } else
  if (isSelectPressed()) {
    // writeAllEepromData(); 
    // stateInt = 1;
    setStateDepartingConfig();
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
  storeMessage(message);
  update();
  readButtons();
  if (isUpPressed()) {
    stateInt = 10;
  } else
  if (isDownPressed()) {
    stateInt = 12;
  } else
  if (isLeftPressed()) {
    decreaseLedOffset();
  } else
  if (isRightPressed()) {
    increaseLedOffset();
  } else
  if (isSelectPressed()) {
    // writeAllEepromData();
    // stateInt = 1;
    setStateDepartingConfig();
  }
  showLightsForRow();
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
  // storeMessage(message);
  // update();
  lcd->clear();
  lcd->print(message);
  lcd->display();
  readButtons();
  if (isUpPressed()) {
    stateInt = 11;
  } else
  if (isDownPressed()) {
    stateInt = 13;
  } else
  if (isLeftPressed()) {
    decreaseLedCount();
    recreateLedStripHandler();
    showLightsForRow();
  } else
  if (isRightPressed()) {
    increaseLedCount();
    recreateLedStripHandler();
    showLightsForRow();
  } else
  if (isSelectPressed()) {
    // writeAllEepromData();
    // stateInt = 1;
    setStateDepartingConfig();
  }
}

/**
display message, and load eeprom row to bitmap handler current row
*/
void uiLoadRowEeprom() {
  currentRow = readEepromRow();
  storeMessage("loading row");
  update();
  delay(3000);
  stateInt = 1;
}

/**
display message, and save current row to eeprom
*/
void uiSaveRowEeprom(int row) {
  String message;
  message += "saving row: ";
  message += String(row + 1);
  storeMessage(message);
  update();
  writeEepromRow(row);
  delay(3000);
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
  lcd->begin(LCD_ROWS, LCD_COLS);

  // stateInt = 16;
  // stateInt = setBitInByte(stateInt, 7, 0x01);
  // displaySimpleMsg((String)getState());
  // delay(2000);
  // displaySimpleMsg((String)stateInt);
  // delay(2000);
  // displaySimpleMsg((String)getBitFromByte(stateInt, 7));
  // delay(2000);
  // uint8_t myInt = setBitInByte(0, 0, 1);
  // displaySimpleMsg((String)myInt);
  // delay(2000);
  // bool myBool = getBitFromByte(myInt, 1);
  // displaySimpleMsg((String)myBool);
  // delay(2000);

  // while(1);

  //initialize the SD card
  initializeCard();

  
  navigateFilesAtRoot();
  // while(1);



  //create bitmap handler object, and pass it the bitmap to read.
  // String fileNameLowerCase = toLowerCase(getFileToOpen());
  
  // closeFile();
  // closeDirectory();
  // File root = SD.open("/");
  // File myFile = root.openNextFile();
  // myFile.close();
  // root.close();
  
  String lowerCaseName = toLowerCase(getFileToOpen());
  // displaySimpleMsg(fileNameLowerCase);
  // delay(1500);
  // //open the file
  openFile(lowerCaseName);
  // //verify file, this includes reading the headers which is necessary to decode the bitmap.
  verifyFile();
  //instantiate light strip handler
  createStrip();
  
  readAllEepromData();

  // //read the current value in eeprom at 0 (the current row number) and print it to lcd display.
  // int myInt = EEPROM.get(EEPROM_OFFSET, myInt);
  // storeMessage(String(myInt));
  // update();
  // delay(3000);
}
void loop() {
  if (!isFileOk()) {
    displaySimpleMsg("file not ok");
    while(1);
    return;
  }
  uiIntro();
  uiDisplayRow();
  uiBrightness();
  uiOffset();
  uiLedCount();
  showLightsForRow();
  uiReset();
  // uiLoadOption();
  // uiNavigateFiles();
  delay(50);
}
