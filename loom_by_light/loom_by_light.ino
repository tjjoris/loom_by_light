/**
*this sketch is intended to load a bitmap from an sd card and display each row of the bitmap's pixels in a 
*addressable light strip. each pixel is converted into one of two values, because the purpose of this is
for a custom loom, which allows the user to manually choose which of 2 colours to use on each loom heddle, 
*thereby creating custom 2d loom images based off a bitmap.
*The program is written by Luke Johnson, commissioned by Elizabeth Johnson, the organizer of the project.
*some of the code was modified after being taken from bitsnbytes.co.uk:
https://bytesnbits.co.uk/bitmap-image-handling-arduino/#google_vignette
*@author Luke Johnson
*@date 2025-January-13
*/

#include <SPI.h>
#include "SD.h"

#define CHIP_SELECT 10
#define NUM_LIGHTS 40

class BitmapHandler {
  //instance variables
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
    *attempts to open file, and if opened, reads the headers, then checks the headers, then prints ok message.
    */
    BitmapHandler(String filename){
      this->fileOK = false;
      this->bmpFilename = filename;
      this->bmpFile = SD.open(this->bmpFilename, FILE_READ);
      if (!this->bmpFile) {
        Serial.print(F("BitmapHandler : Unable to open file "));
        Serial.println(this->bmpFilename);
        this->fileOK = false;
      }
      else {
        if (!this->readFileHeaders()){
          Serial.println(F("Unable to read file headers"));
          this->fileOK = false;
        }
        else {
          if (!this->checkFileHeaders()){
            Serial.println(F("Not compatible file"));
            this->fileOK = false;
          }
          else {
            // all OK
            Serial.println(F("BMP file all OK"));
            this->fileOK = true;
          }       
        }
        // close file
        this->bmpFile.close();
      }
    }

    /**
    *read file headers from file and set instance variables.
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

        // close file
        return true;
      }
      else {
        return false;
      }
    }

    /**
    *check file header values are valid.
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
  *is passed the lights array and passed the row number, returns an array of bytes, representing if each pixel is true or
  *false in that row.
  */
  void setLightsArray(int pixelRow) {
    uint8_t pixelBuffer[3];
    int pixelBufferSize = 3;
    int pixelBufferCounter = sizeof(pixelBuffer);
    int numLightsCounter =0;
    int lightsArrayCounter = 0;
    int shiftInByte = 0;
    int initialBinaryShift = 0;
    int numEmptyBytesPerRow = ((4 - ((3 * this->imageWidth) % 4)) % 4);
    int bytesPerRow;
    byte byteForLightsArray = 0;
    // int displayedWidth, displayedHeight;
    int pixelCol = 0;
    uint32_t pixelRowFileOffset;
    uint8_t r,g,b;
    if (!this->fileOK) {
      return false;
    }
    if (NUM_LIGHTS < imageWidth) {
      Serial.println("image width greater than number of lights.");
      return false;
    }
    if (lightsArraySize < NUM_LIGHTS / 8) {
      Serial.println("lights array size was too small");
    }
    // bytes per row rounded up to next 4 byte boundary
    bytesPerRow = (3 * this->imageWidth) + numEmptyBytesPerRow;
      
    // open file
    this->bmpFile = SD.open(this->bmpFilename, FILE_READ);

    //find the initial binary shift
    initialBinaryShift = 8 - imageWidth;
    if (initialBinaryShift < 0) {
      initialBinaryShift = 0;
    }
    // image stored bottom to top, screen top to bottom
    pixelRowFileOffset = this->imageOffset + ((this->imageHeight - pixelRow - 1) * bytesPerRow);

    //debug
    
      // Serial.print(" pixel row file offset ");
      // Serial.print(pixelRowFileOffset);
      // Serial.print(" bytes per row ");
      // Serial.print(bytesPerRow);
      // Serial.print(" pixel row ");
      // Serial.print(pixelRow);

    for (numLightsCounter = 0; numLightsCounter < NUM_LIGHTS; numLightsCounter++) {
      //set read position to pixel row offset.
      this->bmpFile.seek(pixelRowFileOffset);

      

      // pixel buffer filled, reset pixel buffer.
      // for (pixelCol = 0; pixelCol < displayedWidth; pixelCol ++) {
      // if (pixelBufferCounter >= sizeof(pixelBuffer) - 1){
      //only read if within image bounds.
      if (pixelCol < imageWidth) {
        //seek at new location
        this->bmpFile.seek(pixelRowFileOffset);
        // need to read more from sd card
        this->bmpFile.read(pixelBuffer, pixelBufferSize);
        // pixelBufferCounter = 0;
        //debug
        // Serial.print(" pixels ");
        // Serial.print(pixelBuffer[0]);
        // Serial.print(pixelBuffer[1]);
        // Serial.print(pixelBuffer[2]);
        //increase seek offset.
        pixelRowFileOffset += pixelBufferSize;
        pixelCol ++;
      }
      // } else {
      //   pixelBufferCounter ++;
      // }

      
      //pixel buffer maxed, save pixel
      // if (pixelBufferCounter >= sizeof(pixelBufferCounter)) {
      // get next pixel colours
      b = pixelBuffer[0];
      g = pixelBuffer[1];
      r = pixelBuffer[2];
      //check if pixel is true. 
      bool pixelTrue = isPixelTrue(b, r, g);
      //debug if pixel true
      // Serial.print(" pixel true ");
      // Serial.print(pixelTrue);

      //debug saturation
      // Serial.print(" saturation ");
      // Serial.print(pixelBuffer[pixelBufferCounter]);
      // Serial.print(" column ");
      // Serial.print(pixelCol);
      // Serial.print(" colour ");
      // Serial.print(pixelBufferCounter);
      // Serial.print(" true ");
      // Serial.print(pixelTrue);
      // Serial.println();
      // if (pixelTrue) {
      //   Serial.print(" pixelCol T");
      //   Serial.println(pixelCol);
      // }
      // Serial.print(" num lights counter ");
      // Serial.print(numLightsCounter);
      pixelTrue = (pixelTrue & (numLightsCounter <= imageWidth - 1));
      //add bit to byte
      byteForLightsArray = byteForLightsArray | (pixelTrue << (shiftInByte + initialBinaryShift));
      shiftInByte ++;
      // lightsArray[lightsArrayCounter]= (lightsArray[lightsArrayCounter] | (pixelTrue << shiftInByte));
      // }

      if (shiftInByte < 7){
        // shiftInByte ++;
      } else {
        //set byte to lights array
        lightsArray[lightsArrayCounter] = byteForLightsArray;
        shiftInByte = 0;
        lightsArrayCounter ++;
        byteForLightsArray = 0;
        initialBinaryShift = 0;
      }
      //debug
      // Serial.print(" bytes per row ");
      // Serial.print(bytesPerRow);
      // Serial.print(" pixel row ");
      // Serial.print(pixelRow);
      // Serial.print(" pixel row file offset ");
      // Serial.print(pixelRowFileOffset);
      // Serial.print(" pixel buffer " );
      // Serial.print(pixelBufferCounter);
      // Serial.print(" shiftInByte ");
      // Serial.print(shiftInByte);
      // Serial.print(" lightsArrayCounter ");
      // Serial.print(lightsArrayCounter);
      // Serial.print(" pixelCol ");
      // Serial.println(pixelCol);

    }
  }

  /**
  *is passed 3 colors, checks the combined saturation and returns if the pixel is true or false.
  */
  bool isPixelTrue(uint8_t blue, uint8_t red, uint8_t green) {
    uint8_t total = blue + red + green;
    // Serial.print(" blue ");
    // Serial.print(blue);
    // Serial.print(" red ");
    // Serial.print(red);
    // Serial.print(" green ");
    // Serial.print(green);
    // Serial.print (" total ");
    // Serial.print(total);
    int redInt = red;
    int blueInt = blue;
    int greenInt = green;
    int totalInt = redInt + blueInt + greenInt;
    // Serial.print(" blue int ");
    // Serial.print(blueInt);
    // Serial.print(" green int ");
    // Serial.print(green);
    // Serial.print(" total int ");
    // Serial.print(totalInt);
    // Serial.print(" red int ");
    // Serial.print(redInt);
    // Serial.println();
    if (totalInt >= 384) {
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
*prints a byte as a binary string, source: 
*https://forum.arduino.cc/t/from-byte-to-binary-conversion-solved/341856/5
*/
void printBinary(byte b) {
   for (int i = 7; i >= 0; i-- )
  {
    Serial.print((b >> i) & 0X01);//shift and select first bit
  }
}

void setup() {
  // put your setup code here, to run once:
  //set serial to dispaly on ide
  Serial.begin(9600);
  initializeCard();
  //create bitmap handler object, and pass it the bitmap to read.
  BitmapHandler bmh = BitmapHandler("bitmap.bmp");
  bmh.serialPrintHeaders();
  for (int j=0; j< bmh.imageHeight; j++) {
    //print a row in of the pixel
    bmh.setLightsArray(j);
    //print lights array.
    
    for (int i = 0; i<bmh.lightsArraySize; i++) {
      printBinary(bmh.lightsArray[i]);
    }
    Serial.println();
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
