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

#include <stdlib.h>
#include "../InstanceHelp.h"

const int BOARDSIZE = 33 ;
const int NOMOVES = 31 ;
const int NOTRANSITIONS = 76 ;

const int transitions[][4] = { 
{2,0,4,0},{2,0,2,2},            //A
{3,0,3,2},                      //A
{4,0,2,0},{4,0,4,2}, // 4       //A
  
{2,1,4,1},{2,1,2,3},            //A
{3,1,3,3},                      //A
{4,1,2,1},{4,1,4,3}, // 9       //A
  
{0,2,0,4},{0,2,2,2},                     //B
{1,2,3,2},{1,2,1,4},                     //B
{2,2,2,0},{2,2,4,2},{2,2,2,4},{2,2,0,2}, //E
{3,2,3,0},{3,2,5,2},{3,2,3,4},{3,2,1,2}, //E
{4,2,4,0},{4,2,6,2},{4,2,4,4},{4,2,2,2}, //E
{5,2,3,2},{5,2,5,4},                     //C
{6,2,6,4},{6,2,4,2},//29                 //C
  
  {0,3,2,3},                               //B
{1,3,3,3},                               //B
{2,3,0,3},{2,3,2,1},{2,3,2,5},{2,3,4,3},       //E
{3,3,1,3},{3,3,3,1},{3,3,3,5},{3,3,5,3}, //39  //E
{4,3,2,3},{4,3,4,1},{4,3,4,5},{4,3,6,3},       //E
{5,3,3,3},                               //C
{6,3,4,3}, //45                          //C
  
{0,4,0,2},{0,4,2,4},                     //B
{1,4,3,4},{1,4,1,2},                     //B
{2,4,0,4},{2,4,2,2},{2,4,4,4},{2,4,2,6}, //E
{3,4,1,4},{3,4,3,2},{3,4,5,4},{3,4,3,6}, //57 //E
{4,4,2,4},{4,4,4,2},{4,4,6,4},{4,4,4,6}, //E
{5,4,3,4},{5,4,5,2},                     //C
{6,4,6,2},{6,4,4,4},                     //C
  
{2,5,4,5},{2,5,2,3},                     //D
{3,5,3,3},                               //D
{4,5,2,5},{4,5,4,3},                     //D
  
{2,6,4,6},{2,6,2,4},                     //D
{3,6,3,4},                               //D
{4,6,2,6},{4,6,4,4}                      //D
};

int BoardPos[][2]={
  {0,2},{0,3},{0,4}, //2
  {1,2},{1,3},{1,4}, //5
  {2,0},{2,1},{2,2},{2,3},{2,4},{2,5},{2,6}, //12
  {3,0},{3,1},{3,2},{3,3},{3,4},{3,5},{3,6}, //19
  {4,0},{4,1},{4,2},{4,3},{4,4},{4,5},{4,6}, //26
  {5,2},{5,3},{5,4}, //29
  {6,2},{6,3},{6,4} //32
};

int getBoardNum(int a,int b) {
  int loop;
  int returnVal=-1;
  for(loop=0;loop<BOARDSIZE;loop++) {
	if(a==BoardPos[loop][0] && b==BoardPos[loop][1]) returnVal=loop;
  }
  return returnVal;
}

int getTransitionNum(int x1,int y1,int x2,int y2) {
  int loop;
  int returnVal=-1;
  for(loop = 0;loop < NOTRANSITIONS; loop++) {
	if(x1==transitions[loop][0] &&
	   y1==transitions[loop][1] &&
	   x2==transitions[loop][2] &&
	   y2==transitions[loop][3])
	  returnVal=loop;
  }
  return returnVal;
}

void push_back_if_valid(vector<int>& v, int x1, int y1, int x2, int y2)
{
  if(getTransitionNum(x1, y1, x2, y2) != -1) 
	v.push_back(getTransitionNum(x1, y1, x2, y2));
}
/*
 void push_back_if_valid_symmetric(vector<int>& v, int x1, int y1, int x2, int y2)
 {
   push_back_if_valid(v,x1,y1,x2,y2);
   push_back_if_valid(v,x2,y2,x1,y1);
 }*/

vector<vector<Var> > boards(NOMOVES + 1);
vector<vector<Var> > transition_vars(NOMOVES);
vector<vector<Var> > zero2one(NOMOVES);
vector<vector<Var> > one2zero(NOMOVES);
vector<vector<Var> > unchanged(NOMOVES);


/*
 0,1,2
 3,4,5
 6,7,8,9,0,1,2,
 3,4,5,6,7,8,9,
 0,1,2,3,4,5,6,
 7,8,9,
 0,1,2,
 Trans: 0,1,3,4,8,9,
 
 */
int main (int argc, char * const argv[]) {
  cout << "MINION 1" << endl;
  instance CSP;
  int start_boardpos;
  if(argc == 1)
	start_boardpos = 16;
  else
	start_boardpos = atoi(argv[1]);
  //cout << "START:" << start_boardpos << endl;
  
  // Board Positions
  CSP.bools += (NOMOVES + 1) * BOARDSIZE;
  
  for(int i=0; i < NOMOVES + 1; ++i)
	for(int j=0; j < BOARDSIZE; ++j)
	  boards[i].push_back(Var(Bool, i * BOARDSIZE + j));
  
  // transition vars
  int transition_vars_start = CSP.bools;
  CSP.bools += NOMOVES * NOTRANSITIONS;
  
  for(int i = 0; i < NOMOVES; ++i)
	for(int j = 0; j < NOTRANSITIONS; ++j)
	  transition_vars[i].push_back(Var(Bool, transition_vars_start + i*NOTRANSITIONS + j));
  
  // zero2one vars
  int zero2one_vars_start = CSP.bools;
  CSP.bools += NOMOVES * NOTRANSITIONS;
  for(int i = 0; i < NOMOVES; ++i)
	for(int j = 0; j < NOTRANSITIONS; ++j)
	  zero2one[i].push_back(Var(Bool, zero2one_vars_start + i*NOTRANSITIONS + j));
  
  // one2zero vars
  int one2zero_vars_start = CSP.bools;
  CSP.bools += NOMOVES * NOTRANSITIONS;
  for(int i = 0; i < NOMOVES; ++i)
	for(int j = 0; j < NOTRANSITIONS; ++j)
	  one2zero[i].push_back(Var(Bool, one2zero_vars_start + i*NOTRANSITIONS + j));
  
  // unchanged vars
  int unchanged_vars_start = CSP.bools;
  CSP.bools += NOMOVES * NOTRANSITIONS;
  for(int i = 0; i < NOMOVES; ++i)
	for(int j = 0; j < NOTRANSITIONS; ++j)
	  unchanged[i].push_back(Var(Bool, unchanged_vars_start + i*NOTRANSITIONS + j));
  

  
  CSP.discrete.push_back(BoundsList(0, NOTRANSITIONS, NOMOVES));
  //  int single_transition_vars = CSP.bool;
  
  cout << "# Solitaire reverse, based from position:" << start_boardpos << endl;
    
  for(int v = 0; v < NOTRANSITIONS; ++v)
    printf("#%d:(%d,%d),(%d,%d)\n", v, transitions[v][0], transitions[v][1], transitions[v][2], transitions[v][3]);
  
  CSP.print_vars();
  
  // Todo : var_order;
  vector<Var> var_order;
  vector<char> val_order;
  for(int i = 0; i < NOMOVES; ++i)
  {
    for(int j = 0; j < NOTRANSITIONS; ++j)
	{
	  var_order.push_back(transition_vars[i][j]);
	  val_order.push_back('d');
	}
  }
  CSP.print_var_order(var_order);
  CSP.print_val_order(val_order);  
  
  cout << "1" << endl;
  cout << "[x" << CSP.bools << endl;
  for(int i = CSP.bools + 1; i < CSP.bools + NOMOVES; ++i)
    cout << ",x" << i;
    cout << "]" << endl;
    

  cout << "0 0" << endl;
  
  CSP.optimise_none();
  
  cout << "print v0" << endl;
  
  for(int i=0;i<NOMOVES;++i)
  {
	CSP.constraint(SumLeq, transition_vars[i], Var(Constant, 1));
	CSP.constraint(SumGeq, transition_vars[i], Var(Constant, 1));
  }
  
  
  for(int i=0;i<BOARDSIZE;++i)
  {
	if(i == start_boardpos)
	{ 
	  CSP.constraint(Eq, boards[0][i], Var(Constant, 0));
	  CSP.constraint(Eq, boards[NOMOVES][i], Var(Constant, 1));
	  // add_constraint(UnaryEqualCon(boards[0][i], compiletime_val<0>())); 
	  // add_constraint(UnaryEqualCon(boards[NOMOVES][i], compiletime_val<1>()));
	}
	else
	{
	  CSP.constraint(Eq, boards[0][i], Var(Constant, 1));
	  CSP.constraint(Eq, boards[NOMOVES][i], Var(Constant, 0));
	  //add_constraint(UnaryEqualCon(boards[0][i], compiletime_val<1>())); 
	  //add_constraint(UnaryEqualCon(boards[NOMOVES][i], compiletime_val<0>()));
	}
  }
  
  
  for(int i = 0; i < NOMOVES; ++i)
  {
	for(int loop1 = 0; loop1 < 7; ++loop1)
	  for(int loop2 = 0; loop2 < 7; ++loop2)
	  {
		int curBoardPos = getBoardNum(loop1, loop2);
		if(curBoardPos != -1)
		{
		  
		  // This is in a {} pair just to hide local variables
		{
		  vector<int> v;
		  push_back_if_valid(v,loop1-2,loop2,loop1,loop2);
		  push_back_if_valid(v,loop1+2,loop2,loop1,loop2);
		  push_back_if_valid(v,loop1,loop2-2,loop1,loop2);
		  push_back_if_valid(v,loop1,loop2+2,loop1,loop2);
		  //D_ASSERT(v.size() != 0);
		  vector<Var> w(v.size());
		  for(int loop = 0; loop < v.size(); loop++)
		  { w[loop] = transition_vars[i][v[loop]]; }
		  CSP.constraintReify(zero2one[i][curBoardPos], SumGeq, w, Var(Constant, 1));
		  //	    add_constraint(rareifyCon(BoolGreaterEqualSumCon(w, compiletime_val<1>()), zero2one[i][curBoardPos]));
		}
		{
			vector<Var> and_vars;
			and_vars.push_back(Var(NotBool, boards[i][curBoardPos].val));
			and_vars.push_back(boards[i+1][curBoardPos]);
			CSP.constraint(Min, and_vars, zero2one[i][curBoardPos]);
			// add_constraint(AndCon(VarNotRef(boards[i][curBoardPos]),boards[i+1][curBoardPos], zero2one[i][curBoardPos]));
		  }
				  
		  {
			vector<int> v;
			push_back_if_valid(v,loop1-1,loop2,loop1+1,loop2);
			push_back_if_valid(v,loop1+1,loop2,loop1-1,loop2);
			push_back_if_valid(v,loop1,loop2-1,loop1,loop2+1);
			push_back_if_valid(v,loop1,loop2+1,loop1,loop2-1);
			push_back_if_valid(v,loop1,loop2,loop1+2,loop2);
			push_back_if_valid(v,loop1,loop2,loop1-2,loop2);
			push_back_if_valid(v,loop1,loop2,loop1,loop2+2);
			push_back_if_valid(v,loop1,loop2,loop1,loop2-2);
			vector<Var> w(v.size());
			for(int loop = 0; loop < v.size(); loop++)
			{ w[loop] = transition_vars[i][v[loop]]; }	      
			CSP.constraintReify(one2zero[i][curBoardPos], SumGeq, w, Var(Constant,1));
			//add_constraint(rareifyCon(BoolGreaterEqualSumCon(w, compiletime_val<1>()), one2zero[i][curBoardPos]));
			
		  }
		  {
			vector<Var> and_vars;
			and_vars.push_back(boards[i][curBoardPos]);
			and_vars.push_back(Var(NotBool, boards[i+1][curBoardPos].val));
			CSP.constraint(Min, and_vars, one2zero[i][curBoardPos]);
			// add_constraint(AndCon(boards[i][curBoardPos],VarNotRef(boards[i+1][curBoardPos]), one2zero[i][curBoardPos]));
		  }
		  
		  {
		    vector<Var> and_vars;
			and_vars.push_back(Var(NotBool, zero2one[i][curBoardPos].val));
			and_vars.push_back(Var(NotBool, one2zero[i][curBoardPos].val));
			CSP.constraint(Min, and_vars, unchanged[i][curBoardPos]);
			//add_constraint(AndCon(VarNotRef(zero2one[i][curBoardPos]), VarNotRef(one2zero[i][curBoardPos]), unchanged[i][curBoardPos]));		  
		  }
		  
		  
		  CSP.constraintReify(unchanged[i][curBoardPos], Eq, boards[i][curBoardPos], boards[i+1][curBoardPos]);
		  //add_constraint(EqualCon(boards[i][curBoardPos], boards[i+1][curBoardPos], unchanged[i][curBoardPos]));
		}
	  }
  }
  
  for(int i = 0; i < NOMOVES; ++i)
    for(int j = 0; j < NOTRANSITIONS; ++j)
    {
      CSP.constraintReify(transition_vars[i][j], Eq, Var(Discrete, i), Var(Constant, j));
    }
  //CSP.constraint(Eq, transition_vars[1][getTransitionNum(3,3,3,5)], Var(Constant, 1));

  CSP.constraint(Eq, Var(Discrete, 0), Var(Constant, 68));
  CSP.constraint(Eq, Var(Discrete, 1), Var(Constant, 62));
  CSP.constraint(Eq, Var(Discrete, 2), Var(Constant, 24));
  CSP.constraint(Eq, Var(Discrete, 3), Var(Constant, 15));
  CSP.constraint(Eq, Var(Discrete, 4), Var(Constant, 51));
  
  
  

 }

