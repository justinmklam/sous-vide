/* CountDownTimer.h

Hour, second, minute timer. Modified to run on ESP8266 using millis() instead of micros().

Based on http://playground.arduino.cc/Main/CountDownTimer

Creator: Andrew Mascolo
Original date: 11/7/2014

Adapted by: Justin lam
Last edited: 12/29/2016
*/

#ifndef CountdownTimer_h
#define CountdownTimer_h

#if defined(ARDUINO)
#include <Arduino.h>
#endif

class CountDownTimer {
    private:
        uint32_t Watch, _millis, time;
        uint32_t Clock, R_clock;
        bool Reset, Stop, Paused;
        volatile bool timeFlag;

    public:
        CountDownTimer();
        bool RunTimer();
        void ResetTimer();
        void StartTimer();
        void StopTimer();
        void StopTimerAt(unsigned int hours, unsigned int minutes, unsigned int seconds);
        void PauseTimer();
        void ResumeTimer();
        void SetTimer(unsigned int hours, unsigned int minutes, unsigned int seconds);
        void SetTimer(unsigned int seconds);
        int ShowHours();
        int ShowMinutes();
        int ShowSeconds();
        unsigned long ShowMilliSeconds();
        // unsigned long ShowMicroSeconds();
        bool TimeHasChanged();
        bool TimeCheck(unsigned int hours, unsigned int minutes, unsigned int seconds);
};
#endif