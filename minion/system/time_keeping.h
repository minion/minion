// Minion https://github.com/minion/minion
// SPDX-License-Identifier: MPL-2.0

class TimerClass {
  double _internal_cpuStartTime;
  double _internal_sysStartTime;
  double _last_checkTime;
  double start_wallclock;

public:
  TimerClass() {
    cout.setf(ios::fixed);
    startClock();
  }

  void startClock() {
    _internal_cpuStartTime = get_cpuTime();
    _internal_sysStartTime = get_sysTime();
    _last_checkTime = _internal_cpuStartTime;
    start_wallclock = getRaw_wallTime();
  }

  bool checkTimeout(unsigned seconds) {
    return get_cpuTime() - _internal_cpuStartTime >= seconds;
  }

  template <typename Stream>
  void printTimestepWithoutReset(Stream& sout, const char* time_name) {
    sout << time_name << get_cpuTime() - _last_checkTime << endl;
  }

  template <typename Stream>
  void maybePrintTimestepStore(Stream& sout, const char* time_name, const char* store_name,
                               TableOut& tableout, bool toprint) {
    double tempTime = get_cpuTime();
    double diff = tempTime - _last_checkTime;
    if(toprint)
      sout << time_name << diff << endl;
    _last_checkTime = tempTime;
    tableout.set(string(store_name), tostring(diff));
  }

  template <typename Stream>
  void maybePrintFinaltimestepStore(Stream& sout, const char* time_name, const char* store_name,
                                    TableOut& tableout, bool toprint) {
    double time_wallclock = getRaw_wallTime() - start_wallclock;

    double end_cpuTime = get_cpuTime();
    double end_sysTime = get_sysTime();

    maybePrintTimestepStore(sout, time_name, store_name, tableout, toprint);
    if(toprint) {
      sout << "Total Time: " << end_cpuTime - _internal_cpuStartTime << endl;
      sout << "Total System Time: " << end_sysTime - _internal_sysStartTime << endl;
      sout << "Total Wall Time: " << time_wallclock << endl;
      sout << "Maximum RSS (kB): " << getMax_rss() << endl;
    }
    tableout.set(string("TotalTime"), end_cpuTime - _internal_cpuStartTime);
  }
};
