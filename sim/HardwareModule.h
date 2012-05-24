#ifndef _SIMHWRT_HARDWARE_MODULE_H_
#define _SIMHWRT_HARDWARE_MODULE_H_

class HardwareModule {
public:
  virtual ~HardwareModule(){}
  virtual void ClockRise() = 0;
  virtual void ClockFall() = 0;
  virtual void print() {}
  virtual double Utilization() { return 0.; }
  virtual bool ReportUtilization(int& processed, int& max_process) { return false; }

};

#endif // _SIMHWRT_HARDWARE_MODULE_H_
