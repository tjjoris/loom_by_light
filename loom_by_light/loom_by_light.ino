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

class BitmapHandler {
  //instance variables
  private:
  bool fileOK = false; //if file is ok to use
  File bmpFile; //the file itself
  String bmpFilename; //the file name

    public:

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

void setup() {
  // put your setup code here, to run once:
  //set serial to dispaly on ide
  Serial.begin(9600);
  initializeCard();
  //create bitmap handler object, and pass it the bitmap to read.
  BitmapHandler bmh = BitmapHandler("bitmap.bmp");
  bmh.serialPrintHeaders();
}

void loop() {
  // put your main code here, to run repeatedly:

}
