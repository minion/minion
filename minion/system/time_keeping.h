/*
 * Minion http://minion.sourceforge.net
 * Copyright (C) 2006-09
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

class TimerClass {
  double _internal_cpuStart_time;
  double _internal_sysStart_time;
  double _last_check_time;
  double start_wallclock;

public:
  TimerClass() {
    cout.setf(ios::fixed);
    startClock();
  }

  void startClock() {
    _internal_cpuStart_time = get_cpu_time();
    _internal_sysStart_time = get_sys_time();
    _last_check_time = _internal_cpuStart_time;
    start_wallclock = getRaw_wall_time();
  }

  bool checkTimeout(unsigned seconds) {
    return get_cpu_time() - _internal_cpuStart_time >= seconds;
  }

  template <typename Stream>
  void printTimestepWithoutReset(Stream& sout, const char* time_name) {
    sout << time_name << get_cpu_time() - _last_check_time << endl;
  }

  template <typename Stream>
  void maybePrintTimestepStore(Stream& sout, const char* time_name, const char* store_name,
                               TableOut& tableout, bool toprint) {
    double temp_time = get_cpu_time();
    double diff = temp_time - _last_check_time;
    if(toprint)
      sout << time_name << diff << endl;
    _last_check_time = temp_time;
    tableout.set(string(store_name), tostring(diff));
  }

  template <typename Stream>
  void maybePrintFinaltimestepStore(Stream& sout, const char* time_name, const char* store_name,
                                    TableOut& tableout, bool toprint) {
    double time_wallclock = getRaw_wall_time() - start_wallclock;

    double end_cpu_time = get_cpu_time();
    double end_sys_time = get_sys_time();

    maybePrintTimestepStore(sout, time_name, store_name, tableout, toprint);
    if(toprint) {
      sout << "Total Time: " << end_cpu_time - _internal_cpuStart_time << endl;
      sout << "Total System Time: " << end_sys_time - _internal_sysStart_time << endl;
      sout << "Total Wall Time: " << time_wallclock << endl;
      sout << "Maximum RSS (kB): " << getMax_rss() << endl;
    }
    tableout.set(string("TotalTime"), end_cpu_time - _internal_cpuStart_time);
  }
};
