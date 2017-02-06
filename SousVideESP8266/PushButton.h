/*PushButton.h

Purpose: Class for handling switch debouncing

Last edited: August 29, 2016
Author: Justin Lam
*/

#ifndef HEADER_PUSHBUTTON
  #define HEADER_PUSHBUTTON

class PushButton{
  private:
    int switchPin;      // hardware pin on dev board
    int reading;        
    int prev_state;
    int state;
    bool switchPressed;
    unsigned long last_debounce; 
    unsigned long debounce_delay;

  public:
    PushButton(int pin);     // constructor to initialize variables
    bool CheckSwitchPress();  // Returns true if switch is pressed, false if not
};

#endif