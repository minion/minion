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

#include "minion.h"


const int height = 7;
const int width = 7;
const int widthsum = 4;
const int heightsum = 4;
const int scalarsum = 2;
using namespace Controller;


vector<vector<VarNot<BoolVarRef> > > vars(height);

void print_sol(void)
{
  for(int i=0;i<height;++i)
  {
    for(int j=0;j<width;++j)
    {
      D_ASSERT(vars[i][j].isAssigned());
      printf("%d",vars[i][j].getAssignedValue());
    }
    printf("\n");
  }
  printf("\n");
}

int main (int argc, char * const argv[]) {
  Controller::find_all_solutions();
  // set_solution_function(&print_sol);
  for(int i=0;i<height;i++)
    for(int j=0;j<width;j++)
    {
      vars[i].push_back(VarNot<BoolVarRef>(boolean_container.get_new_var()));
    }
      array<VarNot<BoolVarRef>,height> row;
  array<VarNot<BoolVarRef>,width> col;
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
  
  vector<vector<VarNot<BoolVarRef> > > scalar((height*(height-1))/2);
  for(unsigned int i=0;i<scalar.size();i++)
    for(int j=0;j<width;j++)
      scalar[i].push_back(VarNot<BoolVarRef>(boolean_container.get_new_var()));
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
  
  vector<AnyVarRef> var_order(height*width);
  vector<char> val_order(height*width, 1);
  for(int i = 0; i < height; ++i)
    for(int j = 0; j < width; ++j)
	  var_order[i*width + j] = vars[i][j];

  Controller::solve(var_order, val_order);

  D_ASSERT(Controller::solutions == 151200);
  D_ASSERT(nodes == 367038);
  Controller::finish();
}

