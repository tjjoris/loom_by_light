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

class BitmapHandler {
  //instance variables
  private:
  bool fileOK = false; //if file is ok to use
  File bmpFile; //the file itself
  String bmpFileName; //the file name
  
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


}


void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
