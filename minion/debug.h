/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

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

#ifndef DEBUG_H
#define DEBUG_H

#include "system/to_string.h"
#include "constants.h"

// If true, "-crash" was used and execution will crash on error (for debugging)
VARDEF_ASSIGN(bool debug_crash, false);

struct parse_exception : public exception
{
  string error;
  parse_exception(string s) : error(s)
  {}
  
  virtual const char* what() const throw() 
  { return error.c_str(); }
  
  virtual ~parse_exception() throw()
  {}
};

#define D_FATAL_ERROR(s) { D_FATAL_ERROR2(s,  __FILE__, to_string(__LINE__)); abort(); }

// These functions are defined in debug_functions.cpp

void D_FATAL_ERROR2(string s, string file, string line);
void _NORETURN FAIL_EXIT();

struct assert_fail {};

void assert_function(BOOL x, const char* a, const char* f, int line);
// Unlike Asserts, Checks are always enabled.
#define CHECK(x, y) {assert_function(x, y, __FILE__, __LINE__);}

#ifndef NO_DEBUG

#define BOUNDS_CHECK
#define DUMP_TREE

#define D_ASSERT(x) assert_function(x, #x, __FILE__, __LINE__);
#define D_DATA(x) x

enum debug_types
{ DI_SOLVER, DI_SUMCON, DI_BOOLCON, DI_ANDCON, DI_ARRAYAND, DI_QUEUE, DI_REIFY,
  DI_LEXCON, DI_TABLECON, DI_TEST, DI_DYSUMCON, DI_DYNAMICTRIG, DI_DYELEMENT, 
  DI_INTCONTAINER, DI_GACELEMENT, DI_CHECKCON, DI_VECNEQ };
  
#define DEBUG_CASE(x) case x: std::cerr << #x; break;

#ifdef NO_PRINT
#define D_INFO(x,y,z)
#else
// importance: 0 = trivial, 1 = important, 2 = v. important.
  inline void D_INFO(int importance, debug_types x, std::string s)
  {
    std::cerr << importance << ",";
    switch(x)
    {
      DEBUG_CASE(DI_SOLVER);
      DEBUG_CASE(DI_SUMCON);
      DEBUG_CASE(DI_BOOLCON);
      DEBUG_CASE(DI_ARRAYAND);
      DEBUG_CASE(DI_QUEUE);
      DEBUG_CASE(DI_ANDCON);
      DEBUG_CASE(DI_REIFY);
      DEBUG_CASE(DI_LEXCON);
      DEBUG_CASE(DI_TABLECON);
	  DEBUG_CASE(DI_TEST);
	  DEBUG_CASE(DI_DYSUMCON);
	  DEBUG_CASE(DI_DYNAMICTRIG);
	  DEBUG_CASE(DI_DYELEMENT);
	  DEBUG_CASE(DI_INTCONTAINER);
	  DEBUG_CASE(DI_GACELEMENT);
	  DEBUG_CASE(DI_CHECKCON);
	  DEBUG_CASE(DI_VECNEQ);
      default:
	D_FATAL_ERROR("Missing debug case. Go add it to line 125(ish) in debug.h");
    };
    
    std::cerr << ": " << s << std::endl;
  }
#endif

  
#else
#define D_INFO(x,y,z)
#define D_DATA(x)
#define D_ASSERT(x)
#endif
    
    
inline bool DOMAIN_CHECK(BigInt v)
{ return v < DomainInt_Max && v > DomainInt_Min; }

// These are just to catch cases where the user didn't cast to BigInt
// themselves, which makes the function useless.
inline void DOMAIN_CHECK(DomainInt);
inline void DOMAIN_CHECK(int);
inline void DOMAIN_CHECK(unsigned int);
        
#endif //DEBUG_H

