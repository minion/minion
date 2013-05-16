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

// This file deals with general C++ things which aren't specific to Minion.

#ifndef _MINION_SYSTEM_H
#define _MINION_SYSTEM_H

#include "basic_headers.h"

#include "linked_ptr.h"
#include "wrapper.h"
#include "to_string.h"
#include "local_array.h"
#include "tableout.h"
#include "time_keeping.h"

#include "sys_constants.h"
#include "debug.h"
#include "array_functions.h"
#include "trigger_timer.h"
#include "box.h"
#include "test_functions.h"

// from sha1.cpp
std::string sha1_hash(const std::string& s);
#endif
