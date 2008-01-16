/* Minion
* Copyright (C) 2006
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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


namespace Controller
{
  VARDEF_ASSIGN(BOOL finished, false);
  VARDEF_ASSIGN(BOOL failed, false);
  
  /// Called when search is finished. 
  /// This is mainly here so that any debugging instructions which watch for memory problems
  /// don't trigger when search is finished and memory is being cleaned up.
  inline void finish()
  { 
    D_INFO(0,DI_SOLVER,"Cleanup starts");
    finished = true;
  }
  
  VARDEF(jmp_buf g_env);
  
  /// Called whenever search fails.
  /// Anyone can call this at any time. Once the current propagator is left, search will backtrack.
  inline void fail()
  { 
    D_INFO(1,DI_SOLVER,"Failed!");
#ifdef USE_SETJMP
   _longjmp(g_env,1);
#else
    failed = true;
#endif
  }
  
  void lock();
}
