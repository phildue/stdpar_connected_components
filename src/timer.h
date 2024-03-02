#ifndef CPP_TIMER_H__
#define CPP_TIMER_H__

#include <chrono>
struct Timer
{
  std::chrono::time_point<std::chrono::_V2::system_clock,std::chrono::_V2::system_clock::duration> start,stop;

  void Start()
  {
    start = std::chrono::high_resolution_clock::now();
  }

  void Stop()
  {
    stop = std::chrono::high_resolution_clock::now();

  }

  float Elapsed()
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count();
  }
};

#endif  /* TIMER_H__ */
