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
*@date 2025-January-20
*/

#include <SPI.h>
#include <SD.h>


#define CHIP_SELECT 10 //the chip select used for the SD card.
#define LED_COUNT 60 //the number of lights on the light strip.
#define LED_PIN 1 //the pin the data line for the addressable LED strip is connected to.
// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50 // Set BRIGHTNESS to about 1/5 (max = 255)
#define LCD_ROWS 2 //the number of character rows on the lcd screen, this is how many lines fit on the lcd screen.
#define LCD_COLS 16 //the number of character columns on the lcd screen, this is how many characters fit on one line.

/**
forward declaration of classes:
*/
class LblLcdDisplay;

/**
test class header(declaration), this is seperated from implementation to hide implementation details
and improve encapsulation, making it easier to modify the implementation without affecing to ther code which
depends on the class interface, this class is to be deleted
*/
class TestClass {
  private:
    LblLcdDisplay * _lcdDisplay;
  public:
    TestClass(LblLcdDisplay * lcdDisplay) ;
    // void doStuff();
};

/**
test class implementation, to be deleted.
*/
TestClass::TestClass(LblLcdDisplay * lcdDisplay) {
  _lcdDisplay = lcdDisplay;
}

/**
testing class B. this implements Class B withiout a class header.
*/
class ClassB {
  private:
    LblLcdDisplay * _lblLcdDisplay;
  public:
    ClassB (LblLcdDisplay * lblLcdDisplay) {
      _lblLcdDisplay = lblLcdDisplay;
    }
};

/**
lcd class for displaying messages to the lcd display.
*/
class LblLcdDisplay {
  private:
    LiquidCrystal * _lcd;
    String _storedMessage = "";    //the stored message to be compared to if the message needs to be updated.
    String _messageBeingDisplayed = "";  //the message being displayed, if this is different form the stored message, 
    //the lcd needs to be updated.
    String _messageBeingDisplayedSubString = ""; //the sub string of the displayed message, this is used for when
    //the entire message cannot fit on the lcd screen.
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
    set the stored lcd message, this is what update uses to know if it needs to update the screen.
    */
    void storeMessage(String message) {
      _storedMessage = message;
      _messageBeingDisplayed = message;
      _messageBeingDisplayedSubString = message;
      _updateCounter = 0;
      _charCount = 0;
    }

    /**
    test class to test the display with more text than fits on the screen.
    */
    void testDisplay() {
      _lcd->setCursor(0,0);
      _lcd->print("zero one two three four five six seven eight nine ten eleven twelve");
      _lcd->display();
    }

    /**
    check if the displayed message is different from the stored message, and update it if it is.
    */
    void update() {
      
      // Serial.println(_updateCounter);
      //the update counter is at 0, therefore update the lcd screen.
      if (_updateCounter <= 0) {
        _lcd->clear();
        // int charCount = 0; //the character count to use to get the substring.
        //loop for each row in the lcd screen.
        for (int row = 0; row < LCD_ROWS; row ++) {
          // if (!((row > 0) && (_messageBeingDisplayedSubString.length() == _messageBeingDisplayed.length()))) {
          
          if (_messageBeingDisplayedSubString.length() > 0) {
            //set the cursor to the correct row.
            _lcd->setCursor(0, row);
            for (int col = 0; col < LCD_COLS; col ++) {
              // Serial.println(row);
              // Serial.println(_messageBeingDisplayedSubString);
              if (_messageBeingDisplayedSubString.length() > _charCount) {
                _lcd->write(_messageBeingDisplayedSubString[_charCount]);
              }
              // if (_messageBeingDisplayedSubString.length() > 1) {
              if (_charCount < _messageBeingDisplayed.length()) {
                // _messageBeingDisplayedSubString = _messageBeingDisplayedSubString.substring(1);
                _charCount ++;
              }
              // else {
              // // _messageBeingDisplayedSubString = _messageBeingDisplayed;
              //   _charCount = 0;
              // }
            }
          }
          
          // }
          // for (int col = 0; col < LCD_COLS; col ++) {
          // }
        }
        if (_charCount >= _messageBeingDisplayed.length()) {
          _charCount = 0;
        }
        // Serial.println(_messageBeingDisplayedSubString);
        _lcd->display();
      }
      //the update counter has reached the max, reset it to 0 and end the function.
      if (_updateCounter >= _updateCounterMax) {
        //create a substring
        // if (_messageBeingDisplayedSubString.length() > LCD_COLS) {
        //   _messageBeingDisplayedSubString = _messageBeingDisplayedSubString.substring(LCD_COLS);
        // }
        // else {
        //   _messageBeingDisplayedSubString = _messageBeingDisplayed;
        // }
        _updateCounter = 0;
        // _charCount = 0;
        return;
      }
      //the update counter is less than the update counter max, therefore it should increment.
      if (_updateCounter < _updateCounterMax) {
        _updateCounter ++;
        return;
      }
    }


};

/**
LblLedStripHandler - the class to create the NeoPixel strip object, 
and display the specific lights
*/
class LblLedStripHandler {
  public:
    Adafruit_NeoPixel strip;

    LblLedStripHandler() : strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {
      Serial.println("LblLedStripHandler constructor called.");
      strip.begin(); //initialize NeoPixel strip object (REQUIRED)
      strip.show(); //turn off all pixels.
      strip.setBrightness(BRIGHTNESS); //set the brightness.
    }

    /**
    set a pixel to true or false at a specific location
    */
    void setPixel(int pixelIndex, bool isTrue) {
      if (isTrue) {
        strip.setPixelColor(pixelIndex, strip.Color(0,0,255));
        Serial.print("set pixel to true at index ");
        Serial.print(pixelIndex);
      }
      else {
        strip.setPixelColor(pixelIndex, strip.Color(0,0,0));
        Serial.print("set pixel to false at index ");
        Serial.print(pixelIndex);
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
lblInterace - loom by light interface, displays the lcd output on a 16 x 2 display
and reads the input of the buttons. the pin layout can be configured to work with 
a different display.
*/
class LblInterface {
  public:

    // initialize the library by associating any needed LCD interface pin
    // with the arduino pin number it is connected to
    const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
    LiquidCrystal lcd;
    const int charPerRow = 16; //the number of characters in a row on the lcd screen.
    const int charPerCol = 2; //the number of characters in a column on the lcd screen.
    int timer = 0;
    const int timerMax = 3000;
    String completeMessage;//this is the complete message to be held as a reference.
    String cutMessage; //this is the origional message which as had the beginning cut off so it can 
    //continue from a certain point to dispaly more than one screen of text.
    String lastStoredMessage; //this is the message to compare the origional message to to see if the screen needs
    //to be updated.

    //constructor for LblInterface, also calls the constructor for lcd.
    LblInterface() : lcd(rs, en, d4, d5, d6, d7) {
      lcd.begin(charPerRow, charPerCol);
    }
    // define some values used by the panel and buttons
    int buttonPressed     = 0; //this is temporarily used to store the button press.
    int buttonSaved = 0; //this is set to the button number when the button is pressed, after it is set back to 0.
    int buttonToTest = 0; //this is the button to test bool functions on. it is set until the set value is read.
    int adcKeyIn  = 0;
    int answer = 0;
    #define btnRIGHT  4
    #define btnUP     1
    #define btnDOWN   2
    #define btnLEFT   3
    #define btnSELECT 5
    #define btnNONE   0

    /**
    update the lcd, updating the display, and reading inputs.
    */
    int update() {
      if (!completeMessage.equals(lastStoredMessage)) {
        this->lastStoredMessage = completeMessage;
        updateLcd();
      }
      delay(100);
      int input = readLcdButtons();
      return input;
    }

    /**
    set the lcd button instance variable by reading the lcd buttons. The instance variable
    can be used to test if a button has been pressed or not easily.
    */
    void setLcdButton() {
      buttonToTest = readLcdButtons();
    }

    /**
    reset the button to test so it can be used again.
    */
    void resetLcdButton() {
      buttonToTest = 0;
    }

    /**
    test if the lcd button to test is equal to up.
    */
    bool isButtonUp() {
      if (buttonToTest == 1) {
        resetLcdButton();
        return true;
      }
      return false;
    }

    /**
    test if the lcd button to test is equal to down.
    */
    bool isButtonDown() {
      if (buttonToTest == 2) {
        resetLcdButton();
        return true;
      }
      return false;
    }
    // read the buttons
    int readLcdButtons()
    {
      adcKeyIn = analogRead(0);      // read the value from the sensor 
      //my displayed values are:
      //1023 default
      //720/721 far left
      //480 left center
      //131 up
      //307 down
      //0 right center
      //far right 1023 reset

      // we add approx 50 to those values and check to see if we are close
      if (adcKeyIn > 1000) buttonPressed = btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
      // For V1.1 us this threshold
      else if (adcKeyIn < 50)   buttonPressed = btnRIGHT;  
      else if (adcKeyIn < 250)  buttonPressed = btnUP; 
      else if (adcKeyIn < 450)  buttonPressed = btnDOWN; 
      else if (adcKeyIn < 650)  buttonPressed = btnLEFT; 
      else if (adcKeyIn < 850)  buttonPressed = btnSELECT; 

      //this check prevents the button from being repeated if it is held down.
      if (buttonSaved != buttonPressed) {
        buttonSaved = buttonPressed;
        return buttonPressed;
      } 
      return btnNONE;  // when all others fail, return this...
    }


/**
set the string message to display.
*/
  void setMessage(String message) {
    this->completeMessage = message;
    this->cutMessage = message;
  }

/**
clear the lcd, and update it with the string message.
If the message is too long, continue the message after a certain count.
*/
  void updateLcd() {
    int charCount = 0;
    lcd.clear();
    for (int row = 0; row < charPerCol; row++) {
      lcd.setCursor(0, row);
      for (int col = 0; col < charPerRow; col++) {
        if (charCount < cutMessage.length()) {
          lcd.write(cutMessage[charCount]);
          charCount++;
        }
      }
    }
    if (timer >= timerMax) {
      if (charCount < cutMessage.length()) {
        cutMessage = cutMessage.substring(charCount);
      } else {
        cutMessage = completeMessage;
      }
      timer = 0;
    }
    timer ++;
  }
};

/**
This class is for opening the bitmap on the SD card, and decoding it.
it sets the lightsArray of bytes which stores the binary values of a specific
row of pixels of the bitmap which have been converted based on their saturation.
*/
class BitmapHandler {
  //instance variables:
  private:
  bool fileOK = false; //if file is ok to use
  File bmpFile; //the file itself
  String bmpFilename; //the file name

  public:

  int currentRow; //current row of bitmap being displayed.
  const int lightsArraySize = LED_COUNT / 8;
  //the byte array to store lights binary values.
  byte lightsArray[LED_COUNT / 8];
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
    this->fileOK = false;
    this->bmpFilename = filename;
    currentRow = 0;
  }

  void incrementRow() {
    currentRow++;
    if (currentRow >= imageHeight) {
      currentRow = 0;
    }
  }

  void decrementRow() {
    currentRow--;
    if (currentRow < 0) {
      currentRow = imageHeight - 1;
    }
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
  *verify the opened file.
  * code modified after being sourced from: 
  *https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
  */
  bool verifyFile() {
    if (!this->bmpFile) {
      Serial.print(F("BitmapHandler : Unable to open file "));
      Serial.println(this->bmpFilename);
      this->fileOK = false;
      return false;
    }
    if (!this->readFileHeaders()){
      Serial.println(F("Unable to read file headers"));
      this->fileOK = false;
      return false;
    }
    if (!this->checkFileHeaders()){
      Serial.println(F("Not compatible file"));
      this->fileOK = false;
      return false;
    }
    if (LED_COUNT < imageWidth) {
      Serial.println("image width greater than number of lights.");
      this->fileOK = false;
      return false;
    }
    if (lightsArraySize < LED_COUNT / 8) {
      Serial.println("lights array size was too small");
      this->fileOK = false;
    }
    // all OK
    Serial.println(F("BMP file all OK"));
    this->fileOK = true;
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
      Serial.println("file is not Windows 3.1x, 95, NT, ... etc. bitmap file id.");
      return false;
    }
    // must be single colour plane
    if (this->colourPlanes != 1){
      Serial.println("file is not single colour plane");
      return false;
    }
    // only working with 24 bit bitmaps
    if (this->bitsPerPixel != 24){
      Serial.println("is not 24 bit bitmap.");
      return false;
    }
    // no compression
    if (this->compression != 0){
      Serial.println("bitmap is compressed.");
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
    Serial.print(F("filename : "));
    Serial.println(this->bmpFilename);
    // BMP Header
    Serial.print(F("headerField : "));
    Serial.println(this->headerField, HEX);
    Serial.print(F("fileSize : "));
    Serial.println(this->fileSize);
    Serial.print(F("imageOffset : "));
    Serial.println(this->imageOffset);
    Serial.print(F("headerSize : "));
    Serial.println(this->headerSize);
    Serial.print(F("imageWidth : "));
    Serial.println(this->imageWidth);
    Serial.print(F("imageHeight : "));
    Serial.println(this->imageHeight);
    Serial.print(F("colourPlanes : "));
    Serial.println(this->colourPlanes);
    Serial.print(F("bitsPerPixel : "));
    Serial.println(this->bitsPerPixel);
    Serial.print(F("compression : "));
    Serial.println(this->compression);
    Serial.print(F("imageSize : "));
    Serial.println(this->imageSize);
    Serial.print(F("xPixelsPerMeter : "));
    Serial.println(this->xPixelsPerMeter);
    Serial.print(F("yPixelsPerMeter : "));
    Serial.println(this->yPixelsPerMeter);
    Serial.print(F("totalColors : "));
    Serial.println(this->totalColors);
    Serial.print(F("importantColors : "));
    Serial.println(this->importantColors);
  }

  /**
  *open the file, print an error message if it is not opened. return true if the file is opened, otherwise false.
  */
  bool openFile() {
  this->bmpFile = SD.open(this->bmpFilename, FILE_READ);
  if (!this->bmpFile) {
      Serial.print(F("BitmapHandler : Unable to open file "));
      Serial.println(this->bmpFilename);
      this->fileOK = false;
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
  *is passed a row number, sets array of bytes, representing if each pixel is true or
  *false in that row.
  */
  void setLightsArray(int pixelRow) {
    int pixelBufferSize = 3; //the size of the pixel buffer array
    uint8_t pixelBuffer[pixelBufferSize]; //the pixel buffer array which is used to read from the sd card
    int numLightsCounter =0; //counter for each light
    int lightsArrayCounter = 0; //counter for each byte int he lights array
    int shiftInByte = 0; //the needed binary shift to store binary values in the bit.
    int initialBinaryShift = 0; //this is used for the shifting of the initial byte in the lights array.
    int numEmptyBytesPerRow = ((4 - ((3 * this->imageWidth) % 4)) % 4); //the empty bytes in a row, becase a row must be a multiple of 4 bytes.
    int bytesPerRow; //number of bytes in a row.
    byte byteForLightsArray = 0; //the byte used to store bits for the lights array.
    // int displayedWidth, displayedHeight;
    uint32_t pixelRowFileOffset;
    uint8_t r,g,b;
    if (!this->fileOK) {
      return false;
    }
    // bytes per row rounded up to next 4 byte boundary
    bytesPerRow = (3 * this->imageWidth) + numEmptyBytesPerRow;
    //find the initial binary shift
    initialBinaryShift = calculateInitialBinaryShift(imageWidth);

    // image stored bottom to top, screen top to bottom
    pixelRowFileOffset = this->imageOffset + ((this->imageHeight - pixelRow - 1) * bytesPerRow);
    //set reader to seek location based on image offset.
    this->bmpFile.seek(pixelRowFileOffset);
    //loop for each light.
    for (numLightsCounter = 0; numLightsCounter < LED_COUNT; numLightsCounter++) {
      //only read if within image bounds.
      if (numLightsCounter < imageWidth) {
        this->bmpFile.read(pixelBuffer, pixelBufferSize);
      }
      // get next pixel colours
      b = pixelBuffer[0];
      g = pixelBuffer[1];
      r = pixelBuffer[2];
      //check if pixel is true. 
      bool pixelTrue = isPixelTrue(b, r, g);
      //pixel is also only true if within image bounds.
      pixelTrue = (pixelTrue & (numLightsCounter <= imageWidth - 1));
      //add bit to byte
      byteForLightsArray = byteForLightsArray | (pixelTrue << ((7 - shiftInByte - initialBinaryShift)));
      //increment bit in byte shift.
      shiftInByte ++;
      //a byte is complete, add it to the array.
      if (shiftInByte > 7){
        //set byte to lights array
        lightsArray[lightsArrayCounter] = byteForLightsArray;
        //increment lightsArrayCounter, not to be confused with numLightsCounter
        lightsArrayCounter ++;
        //reset byte
        byteForLightsArray = 0;
        initialBinaryShift = 0;
        shiftInByte = 0;
      }
    }
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
  return true if bit at position in byte is 1,
  for byte in array. Note the bit postion goes from left to right.
  */
  bool isTrueForBitInByteArray(int pixelIndex) {
    int byteIndex = pixelIndex / 8;
    int bitInByteIndex = (pixelIndex % (byteIndex * 8));
    byte myByte = lightsArray[byteIndex];
    bool isBitTrue = isBitTrueInByte(myByte, bitInByteIndex);
    return isBitTrue;
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
  Serial.print("beginning initialization of SD card");

  if (!SD.begin(CHIP_SELECT)) {
    Serial.println("SD initialization failed");
    while (1); //infinate loop to force resetting arduino
  }
  Serial.println("SD Initalized successfully"); //if you reach here setup successfully
  Serial.println("----------------------------\n");
}

/**
print a different character if the value is true or false.
*/
void printBool(bool boolToPrint) {
  if (boolToPrint) {
    Serial.write(1);
  }
  else {
    Serial.print(" ");
  }
}

//global class variables:
LblInterface * lblInterface;
BitmapHandler * bmh;
LblLedStripHandler * lblLedStripHandler;
LblLcdDisplay * lblLcdDisplay;

void showLightsForRow() {
  bmh->setLightsArray(bmh->currentRow);
  //debug the row to the terminal.
  printRow();
  Serial.println();
  for (int i=0; i<LED_COUNT; i++) {
    //set the pixel at i, if it is true in lights array at i.
    lblLedStripHandler->setPixel(i, bmh->isTrueForBitInByteArray(i));
    lblLedStripHandler->showStrip();
  }
}

/**
created for debuggin purposes, prints a row to the ide.
gets the row from BitmapHandler and uses print binary to print it.
it does this in order and does not need to know the position of each column on the row.
*/
void printRow() {
  for (int i = 0; i< LED_COUNT; i++) {
    //print the current byte element in the lights array.
    // printBinary(bmh->lightsArray[i]);
    bool isBitTrue = bmh->isTrueForBitInByteArray(i);
    printBool(isBitTrue);
  }
}

// Adafruit_NeoPixel myStrip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/**
*creates bitmap handler object, then opens bitmap file, reads the headers, then loops each row in the bitmap
*and decodes each row, printing the binary values based on saturation.
*/
void setup() {
  //set serial to dispaly on ide. This cannot be used when using the Neopixel Adafruit light strip
  //library, or it interferes with the light strip.
  Serial.begin(9600);
  //loom by light interface object.
  lblInterface = new LblInterface();
  lblLcdDisplay = new LblLcdDisplay(&lblInterface->lcd);
  lblLcdDisplay->storeMessage("hello world");
  for (int i=0; i<60; i++) {
    lblLcdDisplay->update();
    delay(100);
  }
  lblLcdDisplay->storeMessage("one two three four five six seven eight nine ten eleven twelve thirteen fourteen.");
  while(1) {
    lblLcdDisplay->update();
    delay(100);
  }
  // //test loom by light interface.
  // while (1) {
  //   bool loopCondition = true;
  //   lblInterface->setMessage("hello world");
  //   lblInterface->updateLcd();
  //   // lblInterface->update();
  //   while(loopCondition) {
  //     int buttonInupt = lblInterface->readLcdButtons();
  //     if (buttonInupt == 2) {
  //       loopCondition = false;
  //     }
  //   }
  //   lblInterface->setMessage("good day me");
  //   lblInterface->updateLcd();
  //   delay (10000);
  // }

  //initialize the SD card
  initializeCard();
  //create bitmap handler object, and pass it the bitmap to read.
  bmh = new BitmapHandler("bitmap.bmp");

  //instantiate light strip handler
  lblLedStripHandler = new LblLedStripHandler();
  // myStrip.begin();
  // myStrip.show();
  // myStrip.setBrightness(BRIGHTNESS);
  //tes to test light strip is working.
  // int count = 0;
  // while(1) {
  //   // lblLedStripHandler->setPixel(2, false);
  //   // lblLedStripHandler->setPixel(3, true);
  //   // lblLedStripHandler->showStrip();
  //   Serial.print("showing strip test, count");
  //   Serial.println(count);
  //   // delay(10000);
  //   myStrip.setPixelColor(count, myStrip.Color(155, 155, 255));
  //   myStrip.show();
  //   delay (500);

  //   myStrip.setPixelColor(count, myStrip.Color(0, 0, 0));
  //   myStrip.show();
  //   count +=2;
  //   if (count >= LED_COUNT) {
  //     count = 0;
  //   }
  // }

  //open the file
  bmh->openFile();
  //verify file, this includes reading the headers which is necessary to decode the bitmap.
  if (bmh->verifyFile()) {
    //print the headers.
    // bmh->serialPrintHeaders();

    // // loop for each row.
    // for (int j=0; j< bmh->imageHeight; j++) {
    //   //set the lights array.
    //   bmh->setLightsArray(j);
    //   //loop for each column.
    //   for (int i = 0; i<bmh->lightsArraySize; i++) {
    //     //print the current byte element in the lights array.
        
    //     printBinary(bmh->lightsArray[i]);
    //   }
    //   Serial.println(); //new line
    //   // bmh->incrementRow();
    // }
  }
  // bmh->closeFile(); //close the file
}

void loop() {
  //new design:
  //set button input
  //set state function
    //this function checks each state option.
  //set lcd display
  //delay

  //test loom by light interface.
  //i've discovered the lcd dispaly is not showing messages that don't fit on the screen.
    // while (1) {
      bool loopCondition = true;
      String hello = "hello wold, blah blah.";
      lblInterface->setMessage(hello);
      while(1){
        lblInterface->update();
        // delay(10000);
      }
      lblInterface->updateLcd();
      // lblInterface->update();
      while(loopCondition) {
        // int buttonInupt = lblInterface->readLcdButtons();
        lblInterface->setLcdButton();
        if (lblInterface->isButtonDown()) {
          loopCondition = false;
        }
        delay(100);
      }
      lblInterface->setMessage("good day me");
      lblInterface->updateLcd();
      delay (3000);
    // }
  //intro, dispaly widht and height.
  bool keepLooping = 1;
  String message = "Width ";
  // message += bmh->imageWidth;
  // message += " Height: ";
  // message += bmh->imageHeight;
  lblInterface->setMessage(message);
  lblInterface->updateLcd();
  while (keepLooping) {
    lblInterface->setLcdButton();
    if (lblInterface->isButtonUp()) {
      bmh->currentRow = 0;
      keepLooping = 0;
    } else if (lblInterface->isButtonDown()) {
      bmh->currentRow = bmh->imageHeight - 1;
      keepLooping = 0;
    }
    delay(100);
  }
  //display the current row:
  message = "current row ";
  message += bmh->currentRow;
  lblInterface->setMessage(message);
  lblInterface->updateLcd();
  delay(10000);
  //intro, display width and height.
  while(lblInterface->update() != 2) { 
    String message = "Width: ";
    message += bmh->imageWidth;
    message += " Height: ";
    message += bmh->imageHeight;
    lblInterface->setMessage(message);
    lblInterface->update();
  }
  showLightsForRow();
  while(1) {
    String message = "current row:";
    message += bmh->currentRow;
    lblInterface->setMessage(message);
    int input = lblInterface->update();
    if (input == 1) {
      bmh->incrementRow();
      showLightsForRow();
    }
    else if (input == 2) {
      bmh->decrementRow();
      showLightsForRow();
    }
  }

}
