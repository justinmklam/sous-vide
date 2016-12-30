#include "CountDownTimer.h"

CountDownTimer::CountDownTimer() {
    time = millis();
    Clock = 0;
    Reset = false;
    Stop = false;
    Paused = false;
    timeFlag = false;
}

bool CountDownTimer::RunTimer()
{
  static uint32_t duration = 1000; // 1 second
  timeFlag = false;

  if (!Stop && !Paused) // if not Stopped or Paused, run timer
  {
    // check the time difference and see if 1 second has elapsed
    if ((_millis = millis()) - time > duration ) 
    {
      Clock--;
      timeFlag = true;

      if (Clock == 0) // check to see if the clock is 0
        Stop = true; // If so, stop the timer

     // check to see if micros() has rolled over, if not,
     // then increment "time" by duration
      _millis < time ? time = _millis : time += duration; 
    }
  }
  return !Stop; // return the state of the timer
}

void CountDownTimer::ResetTimer()
{
  SetTimer(R_clock);
  Stop = false;
}

void CountDownTimer::StartTimer()
{
  Watch = millis(); // get the initial microseconds at the start of the timer
  Stop = false;
  Paused = false;
}

void CountDownTimer::StopTimer()
{
  Stop = true;
}

void CountDownTimer::StopTimerAt(unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  if (TimeCheck(hours, minutes, seconds) )
    Stop = true;
}

void CountDownTimer::PauseTimer()
{
  Paused = true;
}

void CountDownTimer::ResumeTimer() // You can resume the timer if you ever stop it.
{
  Paused = false;
}

void CountDownTimer::SetTimer(unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  // This handles invalid time overflow ie 1(H), 0(M), 120(S) -> 1, 2, 0
  unsigned int _Sec = (seconds / 60);
  unsigned int _Min = (minutes / 60);

  if(_Sec) minutes += _Sec;
  if(_Min) hours += _Min;

  Clock = (hours * 3600) + (minutes * 60) + (seconds % 60);
  R_clock = Clock;
  Stop = false;
}

void CountDownTimer::SetTimer(unsigned int seconds)
{
 // StartTimer(seconds / 3600, (seconds / 3600) / 60, seconds % 60);
 Clock = seconds;
 R_clock = Clock;
 Stop = false;
}

int CountDownTimer::ShowHours()
{
  return Clock / 3600;
}

int CountDownTimer::ShowMinutes()
{
  return (Clock / 60) % 60;
}

int CountDownTimer::ShowSeconds()
{
  return Clock % 60;
}

unsigned long CountDownTimer::ShowMilliSeconds()
{
  return (_millis - Watch);
}

// unsigned long CountDownTimer::ShowMicroSeconds()
// {
//   return _millis - Watch;
// }

bool CountDownTimer::TimeHasChanged()
{
  return timeFlag;
}

// output true if timer equals requested time
bool CountDownTimer::TimeCheck(unsigned int hours, unsigned int minutes, unsigned int seconds) 
{
  return (hours == ShowHours() && minutes == ShowMinutes() && seconds == ShowSeconds());
}