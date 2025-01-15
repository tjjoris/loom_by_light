/**
*this sketch is intended to load a bitmap from an sd card and display each row of the bitmap's pixels in a 
*addressable light strip as one of two values based on the saturation of the pixel.
*The purpose of this is for a custom loom, which allows the user to manually choose which of 2 colours to use
*on each loom heddle, thereby creating custom 2d loom images based off a bitmap.
*The program is written by Luke Johnson, commissioned by Elizabeth Johnson, the organizer of the project.
*some of the code was modified after being sourced from bitsnbytes.co.uk:
https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*@author Luke Johnson
*@date 2025-January-14
*/

#include <SPI.h>
#include "SD.h"
#include <AddressableLedStrip.h>


#define CHIP_SELECT 10
#define NUM_LIGHTS 144

class BitmapHandler {
  //instance variables:
  private:
  bool fileOK = false; //if file is ok to use
  File bmpFile; //the file itself
  String bmpFilename; //the file name

  public:

  const int lightsArraySize = NUM_LIGHTS / 8;
  //the byte array to store lights binary values.
  byte lightsArray[NUM_LIGHTS / 8];
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
  *constructor, sets instance variables for filename.
  */
  BitmapHandler(String filename){
    this->fileOK = false;
    this->bmpFilename = filename;
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
    if (NUM_LIGHTS < imageWidth) {
      Serial.println("image width greater than number of lights.");
      this->fileOK = false;
      return false;
    }
    if (lightsArraySize < NUM_LIGHTS / 8) {
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
    for (numLightsCounter = 0; numLightsCounter < NUM_LIGHTS; numLightsCounter++) {
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
*prints a byte as a binary string, reference source: 
*https://forum.arduino.cc/t/from-byte-to-binary-conversion-solved/341856/5
*/
void printBinary(byte b) {
   for (int i = 7; i >= 0; i-- )
  {
    bool myBool = ((b >> i) & 0X01);//convert bit we at index to boolean.
    if (myBool) {
      Serial.write(1); //if true write as a character.
    }
    else {
      Serial.print(" "); //if false, leave an empty space.
    }
  }
}

/**
*creates bitmap handler object, then opens bitmap file, reads the headers, then loops each row in the bitmap
*and decodes each row, printing the binary values based on saturation.
*/
void setup() {
  //set serial to dispaly on ide
  Serial.begin(9600);

  //test class
  AddressableLedStrip addressableLedStrip("hello world!!");
  Serial.println(addressableLedStrip.getMyString());

  //infinate loop
  while(1);

  //initialize the SD card
  initializeCard();
  //create bitmap handler object, and pass it the bitmap to read.
  BitmapHandler bmh = BitmapHandler("bitmap.bmp");
  //open the file
  bmh.openFile();
  //verify file, this includes reading the headers which is necessary to decode the bitmap.
  if (bmh.verifyFile()) {
    //print the headers.
    bmh.serialPrintHeaders();
    //loop for each row.
    for (int j=0; j< bmh.imageHeight; j++) {
      //set the lights array.
      bmh.setLightsArray(j);
      //loop for each column.
      for (int i = 0; i<bmh.lightsArraySize; i++) {
        //print the current byte element in the lights array.
        printBinary(bmh.lightsArray[i]);
      }
      Serial.println(); //new line
    }
  }
  bmh.closeFile(); //close the file
}

void loop() {
  // put your main code here, to run repeatedly:

}
