#ifndef AddressableLedStrip_h
#defnine AddressableLedStrip_h

#include "Arduino.h"

class AddressableLedStrip {
  private:
    String _myString;

  public:
    AddressableLedStrip(String myString);
    String getMyString();
}

#endif