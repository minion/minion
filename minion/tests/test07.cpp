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


#define NO_DEBUG
#define WATCHEDLITERALS

#include "minion.h"
#include "constraints/constraint_sum.h"
#include "constraints/constraint_and.h"

const int height = 9;
const int width = 9;
const int widthsum = 3;
const int heightsum = 3;
const int scalarsum = 1;
using namespace Controller;


vector<vector<BoolVarRef> > vars(height);

void setup_variables()
{
  for(int i=0;i<height;i++)
    for(int j=0;j<width;j++)
    {
      vars[i].push_back(boolean_container.get_new_var());
    }
}


void setup_constraints()
{
  array<BoolVarRef,height> row;
  array<BoolVarRef,width> col;
  
  for(int i=0;i<height;i++)
  {
    for(int j=0;j<width;j++)
      col[j]=vars[i][j];
    add_constraint(BoolLessEqualSumCon(col,compiletime_val<widthsum>()));
    add_constraint(BoolGreaterEqualSumCon(col,compiletime_val<widthsum>()));
  }
  
  for(int j=0;j<width;j++)
  {
    for(int i=0;i<height;i++)
      row[i]=vars[i][j];
    add_constraint(BoolLessEqualSumCon(row,compiletime_val<heightsum>()));
    add_constraint(BoolGreaterEqualSumCon(row,compiletime_val<heightsum>()));
  }
  
  vector<vector<BoolVarRef> > scalar((height*(height-1))/2);
  
  for(unsigned int i=0;i<scalar.size();i++)
    for(int j=0;j<width;j++)
      scalar[i].push_back(boolean_container.get_new_var());
      
  int count = 0;
  for(int i=0;i<height;i++)
  {
    for(int j=i+1;j<height;j++)
    {
      for(int k=0;k<height;k++)
	add_constraint(AndCon(vars[i][k],vars[j][k],scalar[count][k]));
      ++count;
    }
  }
  
  D_ASSERT(count == ((height*(height-1))/2));
  
  for(int i=0;i<(height*(height-1))/2;i++)
  {
    for(int j=0;j<width;j++)
      col[j]=scalar[i][j];
    add_constraint(BoolLessEqualSumCon(col,compiletime_val<scalarsum>()));
    add_constraint(BoolGreaterEqualSumCon(col,compiletime_val<scalarsum>()));
  }
}

int main (int argc, char * const argv[]) {
  
  setup_variables();
  setup_constraints();
  
  pair<vector<BoolVarRef>, vector<BOOL> > var_val_order;
  
  var_val_order.first.resize(height*width);
  var_val_order.second.resize(height*width, false);
  
  for(int i = 0; i < height; ++i)
    for(int j = 0; j < width; ++j)
      var_val_order.first[i*height+j] = vars[i][j];
    
  Controller::print_solution = false;
  
  Controller::find_all_solutions();
  
  Controller::initalise_search();
  
  if(!Controller::failed)
    solve(ORDER_STATIC, var_val_order);
  
  printf("%d, %d\n", nodes, Controller::solutions);
  Controller::finish();
}

