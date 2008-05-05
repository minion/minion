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

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

enum TrigType
{ 
  UpperBound, 
  LowerBound, 
  Assigned,
  DomainChanged, 
  DomainRemoval
};

enum BoundType
{
  Bound_Yes,
  Bound_No,
  Bound_Maybe
};

struct EndOfSearch
{};

#ifndef CONTAINER_TYPE
typedef unsigned int BitContainerType;
#else
typedef CONTAINER_TYPE BitContainerType;
#endif

#endif // _CONSTANTS_H
