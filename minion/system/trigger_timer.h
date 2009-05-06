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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#ifndef TRIG_TIMER_H
#define TRIG_TIMER_H

/// This function will cause the boolean passed to be set to 'true' after timeout.
void activate_trigger(volatile bool*, int timeout, bool CPU_time);

// This takes a point to a bool*, which will be switch when ctrl+c is pressed.
void install_ctrlc_trigger(volatile bool*);

#endif
