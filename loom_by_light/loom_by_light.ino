#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#include <LiquidCrystal.h>

/**
Required libraries: 
  LiquidCrystal by Arduino, Adafruit -version last tested: 1.0.7. 
  Adafruit NeoPixel by Adafruit -version last tested: 1.12.4, 
*this sketch is intended to load a bitmap from an sd card and display each row of the bitmap's pixels in a 
*addressable light strip as one of two values based on the saturation of the pixel.
*The purpose of this is for a custom loom, which allows the user to manually choose which of 2 colours to use
*on each loom heddle, thereby creating custom 2d loom images based off a bitmap.
*The program is written by Luke Johnson, commissioned by Elizabeth Johnson, the organizer of the project.
*some of the code was modified after being sourced from bitsnbytes.co.uk:
https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*@author Luke Johnson
*@date 2025-January-22
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

/**
forward declaration of classes:
*/
class LblLcdDisplay;
class StateEngine;
class UiState;
class UiStateInRow;
class UiStateIntro;
class LblButtons;
class LblFileNavigator;

/**
global variables for classes.
*/
LblLcdDisplay * lblLcdDisplay;
LblButtons * lblButtons;
LblFileNavigator * lblFileNavigator;

// /**
// test class header(declaration), this is seperated from implementation to hide implementation details
// and improve encapsulation, making it easier to modify the implementation without affecing to ther code which
// depends on the class interface, this class is to be deleted
// */
// class TestClass {
//   private:
//     LblLcdDisplay * _lcdDisplay;
//   public:
//     TestClass(LblLcdDisplay * lcdDisplay) ;
//     // void doStuff();
// };

// /**
// test class implementation, to be deleted.
// */
// TestClass::TestClass(LblLcdDisplay * lcdDisplay) {
//   _lcdDisplay = lcdDisplay;
// }

/**
lcd class for displaying messages to the lcd display.
*/
class LblLcdDisplay {
  private:
    LiquidCrystal * _lcd;
    String _storedMessage = "";    //the stored message to be written to the lcd screen.
    int _updateCounter = 0; //the counter to determine if the substring should be continued.
    int _updateCounterMax = 25; //the max the counter should go for the substring to be continued.
    int _charCount = 0; //the character count in the message string.

  public:

    /**
    class constructor, initializes lcd
    */
    LblLcdDisplay (LiquidCrystal * lcd) {
      _lcd = lcd;
      _lcd->begin(LCD_COLS, LCD_ROWS);
    }

    /**
    a simple function for displaying a message
    */
    void displaySimpleMsg(String message) {
      _lcd->clear();
      _lcd->setCursor(0,0);
      _lcd->print(message);
      _lcd->display();
    }

    /**
    clear lcd
    */
    void clearLcd() {
      _lcd->clear();
    }

    /**
    display lcd
    */
    void displayLcd() {
      _lcd->display();
    }

    /**
    display a message at row.
    */
    void displayMsgAtRow(String message, int row) {
      _lcd->setCursor(0,row);
      _lcd->print(message);
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
          _lcd->setCursor(0, row);
          for (int col = 0; col < LCD_COLS; col ++) {
            if (_storedMessage.length() > _charCount) {
              _lcd->write(_storedMessage[_charCount]);
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
        _lcd->clear();
        lcdWrite();
        resetCharCount();
        _lcd->display();
      }
      incrementUpdateCounter();
    }
};


/**
this class is used to get button inputs from the lcd and button shield. It's constructor is passed the 
LiquidCrystal object
*/
class LblButtons {
  private:
    LiquidCrystal * _lcd; //the lcd object.
    int _adcKeyIn; //the key input 
    bool _upPressed = false;
    bool _downPressed = false;
    bool _leftPressed = false;
    bool _rightPressed = false;
    bool _selectPressed = false;
    bool _hasInputBeenRead = false; //this is set true when an input is read, and used to determine
    //when pressed bools should be set from true to false in the _isPressesd functions.
  
  public:
    LblButtons(LiquidCrystal * lcd) {
        _lcd = lcd;
    }

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
      return ((lblButtons->isSelectPressed()) || (lblButtons->isLeftPressed()) || (lblButtons->isRightPressed()) || (lblButtons->isUpPressed()) || (lblButtons->isDownPressed()));
    }
};

/**
this class is used to store the names of files in a given directory.
it uses the instance pointer array fileNames to point to strings of file names.
*/
class LblFileNavigator {
  private:
    // String _fileNames[64];
    File _root;
    File _entry;
    String _address;
    int _numFilesInDir = 0;
    int _currentNavigatedFileCount = 0;
    String _fileToOpen;
    String _tempFileName;
  public:

    /**
    get the file to open
    */
    String getFileToOpen() {
      return _fileToOpen;
    }

    /**
    set the address
    */
    void setAddress(String address) {
      this->_address = address;
    }

    /**
    open the directory
    */
    void openDirectory() {
      _root = SD.open(_address);
      String message;
      if (_root) {
        message = "directory opened"
        DEBUG_LN(message);
        // lblLcdDisplay->displaySimpleMsg("opening dir");
        // delay(1000);
      }
      else {
        message = "failed directory open";
        DEBUG_LN(message);
        lblLcdDisplay->displaySimpleMsg("err opening dir");
        delay(3000);
      }
    }

    /**
    close the directory
    */
    void closeDirectory() {
      _root.close();
      String message = "closing dir";
        DEBUG_LN(message);
        // lblLcdDisplay->displaySimpleMsg(message);
        // delay(1000);
    }

    /**
    display files within the directory. then waits for button presses to respond.
    up or down will navigate, right will open a file if it's valid, select will 
    go back to the config.
    */
    void displayFiles() {
      bool loopDisplayFilesCondition = true; //if to continue displaying files.
      while (loopDisplayFilesCondition) {
        openDirectory();//open the directory.
        int fileCount = 0;//file count is the count of displayed files.
        int row = 0;//row is the row displayed on the lcd screen.
        lblLcdDisplay->clearLcd(); //clear the lcd
        while (row < LCD_ROWS) {//only loop if not exceeded rows to display.
          this->nextFile();//opens the next file.
          fileCount ++;//iterates the count of displayed files.
          //checks if the current file is high enough in the navigated file count to 
          //display on the lcd screen.
          if (fileCount > _currentNavigatedFileCount) {
            if (this->isFile()) {//if file is open.
                String fileNameThisRow = this->getFileName();
              if (row == 0) {//if this is the first file in the row, set the temporary file name.
                _tempFileName = this->getFileName();
                fileNameThisRow += "_";
              }
              lblLcdDisplay->displayMsgAtRow(fileNameThisRow, row);//display file on lcd
              row ++;//iterate row.
            } else {//file could not be opened so end loop.
              row = LCD_ROWS;//match loop condition to end loop.
            }
          }
          if (this->isFile()) {//file exists so close file.
            this->closeFile();
          }
        }
        lblLcdDisplay->displayLcd();//display lcd screen.
        //loop to check button presses, return value is outer loop condition.
        loopDisplayFilesCondition = checkButtonPressesInDisplayFiles();
        closeDirectory();//close directory so it can be opened and files freshly iterated again.
      }
    }

    /**
    loop until a button has been pressed, this also controls the loop
    it exists inside, in case a file is loaded, or select is pressed.
    */
    bool checkButtonPressesInDisplayFiles() {
      bool loopButtonCheckingCondition = true;
      while (loopButtonCheckingCondition) {
        lblButtons->readButtons();
        if (lblButtons->isDownPressed()) {
          _currentNavigatedFileCount ++;
          // break;
          loopButtonCheckingCondition = false;
        }
        else if (lblButtons->isUpPressed()) {
          _currentNavigatedFileCount --;
          if (_currentNavigatedFileCount < 0) {
            _currentNavigatedFileCount = 0;
          }
          // break;
          loopButtonCheckingCondition = false;
        }
        else if (lblButtons->isRightPressed()) {
          if (isFileNamevalid()) {
            setFile();
            loopButtonCheckingCondition = false;
            lblLcdDisplay->displaySimpleMsg("name valid");
            delay(1500);
            return false;
          }

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
    void nextFile() {
      _entry = _root.openNextFile();
    }

    /**
    return true if there is a file opened, else return false.
    */
    bool isFile() {
        if (!_entry) {
          return false;
        }
        return true;
    }

    /**
    close the file
    */
    void closeFile() {
      _entry.close();
    }

    /**
    return the string of the name of the opened file
    */
    String getFileName() {
      String fileName = _entry.name();
      return fileName;
    }


    // /**
    // set pointer array of strings to names of file names in passed directory.
    // can i iterate through a file on the sd card without opening it?
    // */
    // void setFileNames() {
    //   _root = SD.open(_address);
    //   int fileCount = 0;
    //   bool loopCondition = true;
    //   _numFilesInDir = 0;
    //   while (loopCondition) {
    //     File entry = _root.openNextFile();
    //     if ((!entry) || (fileCount > 64)) {
    //       _numFilesInDir = fileCount;
    //       break;
    //     }
    //     _fileNames[fileCount] = (String)entry.name();
    //     DEBUG_LN(entry.name());
    //     fileCount ++;
    //     String message;
    //     // message += "+";
    //     message += (String)entry.name();
    //     // message += String(fileCount);
    //     lblLcdDisplay->storeMessage(message);
    //     lblLcdDisplay->update();
    //     delay(1500);

    //     entry.close();
    //   }

      // String message;
      // for (int i=0; i< count; i++) {
      //   message += fileNames[i];
      //   message += " ";
      //   DEBUG_LN(fileNames[i]);
      // }
    // }
    // String getFileNames() {
    //   String message ;
    //   // message += "_";
    //   for (int i=0; i< _numFilesInDir; i++) {
    //     message += (_fileNames[i]);
    //     message += " ";
    //     DEBUG_LN(_fileNames[i]);
    //     message += i;
    //   }
    //   message += (String)_numFilesInDir;
      
    //   return message;
    // }
};


    /**
    file uses file navigator at root.
    */
    void navigateFilesAtRoot() {
      lblFileNavigator->setAddress("/");
      lblFileNavigator->openDirectory();
      lblFileNavigator->displayFiles();
    }


/**
LblLedStripHandler - the class to create the NeoPixel strip object, 
and display the specific lights
*/
class LblLedStripHandler {
  public:
    Adafruit_NeoPixel strip;

    /**
    the constructor, constructs the strip object
    */
    LblLedStripHandler() : strip(ledCount, LED_PIN, NEO_GRB + NEO_KHZ800) {
      DEBUG_LN("LblLedStripHandler constructor called.");
      strip.begin(); //initialize NeoPixel strip object (REQUIRED)
      strip.setBrightness(brightness); //set the brightness.
      strip.show(); //turn off all pixels.
    }

    /**
    recreate the led strip, probably because the led count has changed.
    */
    void recreateStrip() {

    }

    /**
    set the brightness
    */
    void setLedBrightness() {
      strip.setBrightness(brightness);
      showStrip();
    }

    /**
    set a pixel to true or false at a specific location
    */
    void setPixel(int pixelIndex, bool isTrue) {
      if (isTrue) {
        strip.setPixelColor(pixelIndex, strip.Color(0,0,255));
        DEBUG_MSG("set pixel to true at index ");
        DEBUG_MSG(pixelIndex);
      }
      else {
        strip.setPixelColor(pixelIndex, strip.Color(0,0,0));
        DEBUG_MSG("set pixel to false at index ");
        DEBUG_MSG(pixelIndex);
      }
    }

    /**
    display the set lights on the light strip.
    */
    void showStrip() {
      strip.show();
    }
};



/**
This class is for opening the bitmap on the SD card, and decoding it. it nees to open
and verify a file before reading pixels as true or false.
*/
class BitmapHandler {
  //instance variables:
  private:
  bool fileOk = false; //if file is ok to use
  File bmpFile; //the file itself
  String bmpFilename; //the file name
  int _bytesPerRow;
  int _numEmptyBytesPerRow;
  const int _numBytesPerPixel = 3; //the size of the pixel buffer array

  public:

  int currentRow; //current row of bitmap being displayed.
  // BMP header fields
  uint16_t headerField;
  uint32_t fileSize;
  uint32_t imageOffset;
  // DIB header
  uint32_t headerSize;
  uint32_t imageWidth;
  uint32_t imageHeight;
  uint16_t colourPlanes;
  uint16_t bitsPerPixel;
  uint32_t compression;
  uint32_t imageSize;
  uint32_t xPixelsPerMeter;
  uint32_t yPixelsPerMeter;
  uint32_t totalColors;
  uint32_t importantColors;

  
  /**
  *constructor, sets instance variables for filename.
  */
  BitmapHandler(String filename){
    this->fileOk = false;
    this->bmpFilename = filename;
    currentRow = 0;
  }

  bool isFileOk() {
    return this->fileOk;
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
    if (!this->bmpFile) {
      return 0;
    }
    else {
      return this->bmpFile.read();
    }
  }

  /**
  *read 2 bytes, and return them as an unsigned 16 bit int.
  * code sourced from: 
  *https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
  */
  uint16_t read16Bit(){
    uint16_t lsb, msb;
    if (!this->bmpFile) {
      return 0;
    }
    else {
      lsb = this->bmpFile.read();
      msb = this->bmpFile.read();
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
    if (!this->bmpFile) {
      return 0;
    }
    else {
      lsb = this->bmpFile.read();
      b2 = this->bmpFile.read();
      b3 = this->bmpFile.read();
      msb = this->bmpFile.read();
      return (msb<<24) + (b3<<16) + (b2<<8) + lsb;
    }
  }

  /**
  print an error message
  */
  void errorMessage(String message) {
      lblLcdDisplay->storeMessage(message);
      for (int i = 0; i< 60; i++) {
        lblLcdDisplay->update();
        lblButtons->readButtons();
        delay(100);
        if (lblButtons->isAnyButtonPressed()) {
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
    if (!this->bmpFile) {
      String message;
      message = "unable to open file: ";
      message += this->bmpFilename;
      DEBUG_LN(message);
      errorMessage(message);
      this->fileOk = false;
      return false;
    }
    if (!this->readFileHeaders()){
      String message;
      message = "Unable to read file headers";
      DEBUG_LN(message);
      errorMessage(message);
      this->fileOk = false;
      return false;
    }
    if (!this->checkFileHeaders()){
      String message;
      message = "Not compatable file";
      DEBUG_LN(message);
      errorMessage(message);
      this->fileOk = false;
      return false;
    }
    if (ledCount < imageWidth) {
      String message;
      message = "Image width greater then LED count ";
      message += ledCount;
      DEBUG_LN(message);
      errorMessage(message);
      this->fileOk = false;
      return false;
    }
    // all OK
    DEBUG_LN("BMP file all OK");
    this->fileOk = true;
    return true;
  }

  /**
  *read file headers from file and set instance variables.
  * code modified after being sourced from: 
  *https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
  */
  bool readFileHeaders(){
    if (this->bmpFile) {
      // reset to start of file
      this->bmpFile.seek(0);
      
      // BMP Header
      this->headerField = this->read16Bit();
      this->fileSize = this->read32Bit();
      this->read16Bit(); // reserved
      this->read16Bit(); // reserved
      this->imageOffset = this->read32Bit();

      // DIB Header
      this->headerSize = this->read32Bit();
      this->imageWidth = this->read32Bit();
      this->imageHeight = this->read32Bit();
      this->colourPlanes = this->read16Bit();
      this->bitsPerPixel = this->read16Bit();
      this->compression = this->read32Bit();
      this->imageSize = this->read32Bit();
      this->xPixelsPerMeter = this->read32Bit();
      this->yPixelsPerMeter = this->read32Bit();
      this->totalColors = this->read32Bit();
      this->importantColors = this->read32Bit();
      //the empty bytes in a row, becase a row must be a multiple of 4 bytes.;
      this->_numEmptyBytesPerRow = ((4 - ((3 * this->imageWidth) % 4)) % 4); 
      //the number of bytes in a row.
      this->_bytesPerRow = (3 * this->imageWidth) + _numEmptyBytesPerRow;
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
    if (this->headerField != 0x4D42){
      String message = "file is not Windows 3.1x, 95, NT, ... etc. bitmap file id.";
      DEBUG_LN(message);
      errorMessage(message);
      return false;
    }
    // must be single colour plane
    if (this->colourPlanes != 1){
      String message = "file is not single colour plane";
      DEBUG_LN(message);
      errorMessage(message);
      return false;
    }
    // only working with 24 bit bitmaps
    if (this->bitsPerPixel != 24){
      String message = "is not 24 bit bitmap.";
      DEBUG_LN(message);
      errorMessage(message);
      return false;
    }
    // no compression
    if (this->compression != 0){
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
    DEBUG_LN(this->bmpFilename);
    // BMP Header
    // DEBUG_MSG(F("headerField : "));
    // Serial.println(this->headerField, HEX);
    DEBUG_MSG("fileSize : ");
    DEBUG_LN(this->fileSize);
    DEBUG_MSG("imageOffset : ");
    DEBUG_LN(this->imageOffset);
    DEBUG_MSG("headerSize : ");
    DEBUG_LN(this->headerSize);
    DEBUG_MSG("imageWidth : ");
    DEBUG_LN(this->imageWidth);
    DEBUG_MSG("imageHeight : ");
    DEBUG_LN(this->imageHeight);
    DEBUG_MSG("colourPlanes : ");
    DEBUG_LN(this->colourPlanes);
    DEBUG_MSG("bitsPerPixel : ");
    DEBUG_LN(this->bitsPerPixel);
    DEBUG_MSG("compression : ");
    DEBUG_LN(this->compression);
    DEBUG_MSG("imageSize : ");
    DEBUG_LN(this->imageSize);
    DEBUG_MSG("xPixelsPerMeter : ");
    DEBUG_LN(this->xPixelsPerMeter);
    DEBUG_MSG("yPixelsPerMeter : ");
    DEBUG_LN(this->yPixelsPerMeter);
    DEBUG_MSG("totalColors : ");
    DEBUG_LN(this->totalColors);
    DEBUG_MSG("importantColors : ");
    DEBUG_LN(this->importantColors);
  }

  bool fileExists() {
    if (SD.exists(this->bmpFilename)) {
      DEBUG_MSG(F("File exists "));
      DEBUG_LN((this->bmpFilename));
      return true;
    }
    return false;
  }

  /**
  *open the file, print an error message if it is not opened. return true if the file is opened, otherwise false.
  */
  bool openFile() {
  this->bmpFile = SD.open(this->bmpFilename, FILE_READ);
  if (!this->bmpFile) {
      DEBUG_MSG(F("BitmapHandler : Unable to open file "));
      DEBUG_LN(this->bmpFilename);
      this->fileOk = false;
      return false;
    }
    return true;
  }

  /**
  *close the file.
  */
  void closeFile() {
    this->bmpFile.close();
  }

  /**
  return true if light is on at column, first it gets the pixel row offset, based on the base pixel row offset
  and adding the empty filler bytes (because the bytes containing pixel info must be multiples of 4) and also
  the column times 3 for for each pixel in that row. it then sets the read position for the SD reader, then reads
  a buffer of 3 bytes. It then passes the read bytes to isPixelTrue() and gets back if the pixel is 
  true or not, returning that.
  */
  bool isLightOnAtColumn(int column) {
    if ((column < ledOffset) || (column >= (ledOffset + this->imageWidth))) {
      return false;
    }
    uint8_t pixelBuffer[_numBytesPerPixel];
    int pixelRowFileOffset = this->imageOffset + ((column - ledOffset) * _numBytesPerPixel) + ((this->imageHeight - this->currentRow - 1) * _bytesPerRow);
    this->bmpFile.seek(pixelRowFileOffset);
    this->bmpFile.read(pixelBuffer, _numBytesPerPixel);
    return isPixelTrue(pixelBuffer[0], pixelBuffer[1], pixelBuffer[2]);
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
};


/**
*initialize the SD card.
*/
void initializeCard() {
  DEBUG_MSG("beginning initialization of SD card");

  if (!SD.begin(CHIP_SELECT)) {
    String message = "SD initialization failed";
    DEBUG_LN(message);
    lblLcdDisplay->storeMessage(message);
    while (1); //infinate loop to force resetting arduino
  }
  DEBUG_LN("SD Initalized successfully"); //if you reach here setup successfully
  DEBUG_LN("----------------------------\n");
  lblLcdDisplay->storeMessage("SD initialized successfully");
  lblLcdDisplay->update();
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

//global class variables:
BitmapHandler * bmh;
LblLedStripHandler * lblLedStripHandler;
// LblLcdDisplay * lblLcdDisplay;
// LblButtons * lblButtons;
LiquidCrystal * lcd;

/**
delte and recreate the LED strip handler with a strop object, likely because the LED count has changed.
*/
void recreateLedStripHandler() {
  delete lblLedStripHandler;
  lblLedStripHandler = new LblLedStripHandler();
}

void showLightsForRow() {
  for (int i=0; i<ledCount; i++) {
    //set the pixel at i, if it is true in lights array at i.
    lblLedStripHandler->setPixel(i, bmh->isLightOnAtColumn(i));
    lblLedStripHandler->showStrip();
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
    lblLedStripHandler->setPixel(ledCount, false); 
    lblLedStripHandler->showStrip(); //show the strip again to update the removed pixel.
  }
  checkLedOffset();
}

/**
increasse the led offset, if it's above max, set it to 0.
*/
void increaseLedOffset() {
  ledOffset ++;
  if (ledOffset > (ledCount - bmh->imageWidth)) {
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
    ledOffset = (ledCount - bmh->imageWidth);
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
  lblLedStripHandler->setLedBrightness();
}

/**
decrease brightness by a fixed amount, if it is under the min, set it to 255.
*/
void decreaseBrightnessVar() {
  brightness -= BRIGHTNESS_INCREMENT;
  if (brightness <= 0) {
    brightness = 255;
  }
  lblLedStripHandler->setLedBrightness();
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
  if (ledOffset > (ledCount - bmh->imageWidth)) {
    ledOffset = ledCount - bmh->imageWidth;
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
  if ((rowRead <=0) | (rowRead >= bmh->imageHeight)) {
    rowRead = 0;
    EEPROM.put(EEPROM_ROW, rowRead);
  }
  return rowRead;
}

/**
write the row number to the EEPROM, first check if it's within the image bounds.
*/
void writeEepromRow(int row) {
  if ((row <=0) | (row >= bmh->imageHeight)) {
    row = 0;
  }
  EEPROM.put(EEPROM_ROW, row);
}

int stateInt = 0;

/**
the intro menu screen.
*/
void uiIntro() {
  if (stateInt != 0) {
    return;
  }
  String message;
  message = "bmp width: ";
  message += String(bmh->imageWidth);
  message += " height: ";
  message += String(bmh->imageHeight);
  if (lblLcdDisplay) {
    DEBUG_MSG ("object extist ");
    DEBUG_LN((String)message);
  } else {
    DEBUG_LN("object does not exist");
  }
  lblLcdDisplay->storeMessage(message);
  lblLcdDisplay->update();
  lblButtons->readButtons();
  if ((lblButtons->isUpPressed()) || (lblButtons->isDownPressed())) {
    bmh->currentRow = readEepromRow(); //read the current row from the EEPROM.
    stateInt = 1;
  }
  if (lblButtons->isSelectPressed()) {
    stateInt = 100;
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
  message += (bmh->currentRow + 1);
  lblLcdDisplay->storeMessage(message);
  lblLcdDisplay->update();
  showLightsForRow();
  lblButtons->readButtons();
  if (lblButtons->isUpPressed()) {
    bmh->decrementRow();
  } else
  if (lblButtons->isDownPressed()) {
    bmh->incrementRow();
  }
  if (lblButtons->isRightPressed()) {
    uiSaveRowEeprom(bmh->currentRow);
    return;
  }
  if (lblButtons->isLeftPressed()) {
    uiLoadRowEeprom();
    return;
  }
  if (lblButtons->isSelectPressed()) {
    stateInt = 100;
  }
}

/**
the UI for the brigtness in the menu, up goes to the last menu setting, down to the next
left decreases the brightness, right increases the brightness, select goes to row mode.
*/
void uiBrightness() {
  if (stateInt != 100) {
    return;
  }
  String message;
  message = "brightness: ";
  message += String(brightness);
  lblLcdDisplay->storeMessage(message);
  lblLcdDisplay->update();
  lblButtons->readButtons();
  if (lblButtons->isRightPressed()) {
    increaseBrightnessVar();
  } else
  if (lblButtons->isLeftPressed()) {
    decreaseBrightnessVar();
  } else
  if (lblButtons->isUpPressed()) {
    stateInt = 102;
  } else
  if (lblButtons->isDownPressed()) {
    stateInt = 101;
  } else
  if (lblButtons->isSelectPressed()) {
    writeAllEepromData(); 
    stateInt = 1;
  }
}

/**
the led offset setting, display current offset, up goes to previous config, down to next, 
left decreases offset, right increases offset, select goes back to row mode.
*/
void uiOffset() {
  if (stateInt != 101) {
    return;
  }
  String message;
  message = "offset: ";
  message += ledOffset;
  lblLcdDisplay->storeMessage(message);
  lblLcdDisplay->update();
  lblButtons->readButtons();
  if (lblButtons->isUpPressed()) {
    stateInt = 100;
  } else
  if (lblButtons->isDownPressed()) {
    stateInt = 102;
  } else
  if (lblButtons->isLeftPressed()) {
    decreaseLedOffset();
  } else
  if (lblButtons->isRightPressed()) {
    increaseLedOffset();
  } else
  if (lblButtons->isSelectPressed()) {
    writeAllEepromData();
    stateInt = 1;
  }
  showLightsForRow();
}

/**
the ui configure for the led count.
*/
void uiLedCount() {
  if (stateInt != 102) {
    return;
  }
  String message;
  message += "LED count: ";
  message += (String)ledCount;
  lblLcdDisplay->storeMessage(message);
  lblLcdDisplay->update();
  lblButtons->readButtons();
  if (lblButtons->isUpPressed()) {
    stateInt = 101;
  } else
  if (lblButtons->isDownPressed()) {
    stateInt = 100;
  } else
  if (lblButtons->isLeftPressed()) {
    decreaseLedCount();
    recreateLedStripHandler();
    showLightsForRow();
  } else
  if (lblButtons->isRightPressed()) {
    increaseLedCount();
    recreateLedStripHandler();
    showLightsForRow();
  } else
  if (lblButtons->isSelectPressed()) {
    writeAllEepromData();
    stateInt = 1;
  }
}

/**
display message, and load eeprom row to bitmap handler current row
*/
void uiLoadRowEeprom() {
  bmh->currentRow = readEepromRow();
  lblLcdDisplay->storeMessage("loading row");
  lblLcdDisplay->update();
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
  lblLcdDisplay->storeMessage(message);
  lblLcdDisplay->update();
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
  lblLcdDisplay = new LblLcdDisplay(lcd);
  lblButtons = new LblButtons(lcd);
  //initialize the SD card
  initializeCard();

  
  lblFileNavigator = new LblFileNavigator();
  navigateFilesAtRoot();
  // while(1);
  // lblFileNavigator->setAddress("/");
  // lblFileNavigator->setFileNames();
  // delay(3000);
  // lblLcdDisplay->storeMessage(lblFileNavigator->getFileNames());
  // while(1)
  {
    // lblLcdDisplay->update();
    // delay(100);
  }


  //create bitmap handler object, and pass it the bitmap to read.
  String fileNameLowerCase = lblFileNavigator->toLowerCase(lblFileNavigator->getFileToOpen());
  bmh = new BitmapHandler(fileNameLowerCase);
  lblLcdDisplay->displaySimpleMsg(fileNameLowerCase);
  delete lblFileNavigator; //destroy the file navigator object to save memory.
  delay(1500);
  // //open the file
  bmh->openFile();
  // //verify file, this includes reading the headers which is necessary to decode the bitmap.
  bmh->verifyFile();
  //instantiate light strip handler
  lblLedStripHandler = new LblLedStripHandler();
  
  readAllEepromData();

  // //read the current value in eeprom at 0 (the current row number) and print it to lcd display.
  // int myInt = EEPROM.get(EEPROM_OFFSET, myInt);
  // lblLcdDisplay->storeMessage(String(myInt));
  // lblLcdDisplay->update();
  // delay(3000);
}
void loop() {
  if (!bmh->isFileOk()) {
    lblLcdDisplay->displaySimpleMsg("file not ok");
    return;
  }
  uiIntro();
  uiDisplayRow();
  uiBrightness();
  uiOffset();
  uiLedCount();
  delay(50);
}
