#include <algorithm>
#include "../InstanceHelp.h"
#include <fstream>

struct SteelMillInstance
{
  int sigma, k, j;
  vector<int> sizes;
  vector<int> orderWeights, orderColours;
  
  SteelMillInstance(char* fname)
  {
    ifstream infile ;
    infile.open(fname) ;
    if (!infile.good()) {
      cout << "Could not open " << fname << endl ;
      exit(0);
    }
    infile >> sigma ;
    sizes = vector<int>(sigma+1) ;
    sizes[0] = 0 ;
    for (int i = 0; i < sigma; i++)
    {
      int in;
      infile >> in;
      sizes[i+1] = in;
    }
    //cout << sigma << " sizes: ";
	// for(unsigned int i=0;i < sizes.size(); ++i)
	// cout << (int)sizes[i] << " ";
	// cout << endl;
    infile >> k >> j ;
    
    vector<pair<int,int> > input(j);
    for(int i = 0; i < j; i++) {
      infile >> input[i].first >> input[i].second;
    }
    sort(input.begin(),input.end());
    reverse(input.begin(),input.end());
    orderWeights = vector<int>(j) ;
    orderColours = vector<int>(j) ;
    for (int i = 0; i < j; i++) {
      orderWeights[i]=input[i].first;
      orderColours[i]=input[i].second;
    }
    
    k = 0;
    for(int i=0;i<j;i++)
      k = max(k, orderColours[i]);
    ++k;
	// cout << j << " orders, " << k << " colours " << endl ;
	// for (int i = 0; i < j; i++)
	//   cout << orderWeights[i] << " " << orderColours[i] << endl ;
    infile.close() ;
  }
  
  int getTotalOrderWeight() const {
    int totalOrderWeight = 0 ;
    for (int i = 0; i < j; i++)
      totalOrderWeight += orderWeights[i] ;
    return totalOrderWeight ;
  }
  
  int getMaxOrderSize() const {
    int maxOrderSize = 0;
    for(int i=0;i < j; ++i)
      maxOrderSize = max(maxOrderSize, orderWeights[i]);
    return maxOrderSize;
  }
  
  int getMaxWasteage() const {
    int maxWasteage = 0;
    int maxIndivWasteage = 0;
    for (int i = 0; i < j; i++) {
      bool found = false ;
      for (int i2 = 1; i2 <= sigma ; i2++) {
		if (!found && sizes[i2] >= orderWeights[i]) {
		  int indivWasteage = sizes[i2] - orderWeights[i] ;
		  maxWasteage += indivWasteage ;
		  if (indivWasteage > maxIndivWasteage)
			maxIndivWasteage = indivWasteage ;
		  found = true ;
		}
      }
    }
    
    return maxWasteage;
  }
};


int main(int argc, char** argv)
{
  instance CSP;
    
  if(argc != 2)
  {
			 cerr << "usage: steelmill <name of instance>" << endl;
			 exit(0);
  }
  cout << "MINION 1" << endl;
   SteelMillInstance smi(argv[1]);
  
  
  int i = 0, i2 ;
  int totalOrderWeight = smi.getTotalOrderWeight() ;
  int minSlabs = totalOrderWeight / smi.sizes[smi.sigma] ;
  if (totalOrderWeight % (int)smi.sizes[smi.sigma] != 0)
			 minSlabs++ ;
  
  
  int maxWasteage = smi.getMaxWasteage() ;
  
  // Make Variables
  //Slab variables
  i=0;
  while(smi.sizes[i] < smi.getMaxOrderSize())
	++i;
  
  vector<int> reduced_sizes(smi.sizes.begin() + 1, smi.sizes.end());
  vector<int> first_slab(smi.sizes.begin() + i, smi.sizes.end());
  
  CSP.sparse_bound.push_back(SparseList(first_slab,1));
  CSP.sparse_bound.push_back(SparseList(reduced_sizes, minSlabs-1));
  CSP.sparse_bound.push_back(SparseList(smi.sizes, smi.j - minSlabs));
  
  // Total weight var
  CSP.bound.push_back(BoundsList(totalOrderWeight, totalOrderWeight + maxWasteage, 1));  
  
  //Order Matrix
  CSP.bools += smi.j * smi.j;
  // reification of lex on rows
  int start_of_reification_bools = CSP.bools;
  CSP.bools += smi.j - 1;
  
  //Colour Matrix: Rows
  int start_of_colour_matrix = CSP.bools;
  CSP.bools += smi.k * smi.j;
  
  // End of setting up variables.
  cout << "#SteelMill Instance - " + string(argv[1]) << endl;
  CSP.print_vars();
  
  vector<Var> var_order;
  vector<char> val_order;
  
  // total size
  var_order.push_back(Var(Bound,0));
  val_order.push_back('a');
  // Slabs
  for(int i = 0; i < smi.j; ++i)
  {
	var_order.push_back(Var(SparseBound, i));
	val_order.push_back('a');
  }
  // Orders
  for(i=0;i<smi.j;i++)
  {
	for(i2=0;i2<smi.j;i2++)
	{
	  var_order.push_back(Var(Bool, i2 * smi.j + i));
	  //add_to_var_order(smi.OA[i2*smi.j+i]);
	  val_order.push_back('d');
	}
  }  
  
  CSP.print_var_order(var_order);
  CSP.print_val_order(val_order);
  
  // No vectors.
  cout << "0" << endl;
  
  // 1 matrix, the solution!
  cout << "1" << endl;
  cout << "[" << endl;
  // Total Capacity
  cout << "[" << CSP.str(Var(Bound, 0)) << "]";
  
 
  for(int i = 0 ; i < smi.j; ++i)
  {
	cout << "," << endl;
    cout << "[ ";
    cout << CSP.str(Var(SparseBound, i)) << ",";
	cout << CSP.str(Var(Bool, i * smi.j)) ;
	for(int j = 1; j < smi.j ; ++j)
	  cout << "," << CSP.str(Var(Bool, i * smi.j + j));
	cout << "]";
  }
  cout << endl << "]" << endl;
  // No tensors at the moment.
  cout << " 0" << endl;
  
  CSP.optimise_min(Var(Bound,0));
  
  cout << "print m0" << endl;
  
  // Constraints on Slabs.
  vector<Var> SlabSizeVars;
  for(int i=0; i < smi.j; ++i)
	SlabSizeVars.push_back(Var(SparseBound, i));	
  
  //Slab symmetry breaking
  for (i = 0; i < smi.j-1; i++)
	CSP.constraint(Ineq, Var(SparseBound, i+1), Var(SparseBound, i), Var(Constant,0));
  
  
  CSP.constraint(SumLeq, SlabSizeVars, Var(Bound, 0));
  CSP.constraint(SumGeq, SlabSizeVars, Var(Bound, 0));
  
  
  // Order matrix constraints
  for (i = 0; i < smi.j; i++) {
			 vector<Var> orderMCol;
			 //    vector<BoolVarRef> orderMCol;
			 for (i2 = 0; i2 < smi.j; i2++)
			   orderMCol.push_back(Var(Bool, i2*smi.j + i));
			 //      orderMCol.push_back(smi.OA[i2*smi.j+i]) ;
			 CSP.constraint(SumLeq, orderMCol, Var(Constant, 1));
			 CSP.constraint(SumGeq, orderMCol, Var(Constant, 1));
			 //    add_constraint(BoolLessEqualSumCon(orderMCol, compiletime_val<1>()));
			 //    add_constraint(BoolGreaterEqualSumCon(orderMCol, compiletime_val<1>()));
  }
  
  for (i = 0; i < smi.j; i++) {   
			 vector<Var> scalarRow;
			 for(i2 = 0; i2 < smi.j ; i2++)
			   scalarRow.push_back(Var(Bool, i*smi.j+i2));
			 CSP.constraintScalarLeq(smi.orderWeights, scalarRow,  Var(SparseBound, i));
			 //    add_constraint(LeqWeightBoolSumCon(scalarRow, smi.orderWeights, smi.S[i]));
  }
  
  
  //Sym breaking: GACLexLeq on rows where slabs are equal. 
  
  
  for (i= 0; i < smi.j-1; i++) {
			 vector<Var> orderMRow ;
			 vector<Var> orderMRow2 ;
			 for (i2 = 0; i2 < smi.j; i2++) {
			   orderMRow.push_back(Var(Bool, i*smi.j+i2)) ;
			   orderMRow2.push_back(Var(Bool, (i+1)*smi.j+i2)) ;
			 }
			 
			 CSP.constraintReify(Var(Bool, start_of_reification_bools + i), Eq, Var(SparseBound, i), Var(SparseBound, i+1));
			 CSP.constraintReifyTrue(Var(Bool, start_of_reification_bools + i), LexLeq, orderMRow2, orderMRow);
  }
  
  //Sym-breaking: GAC-Lex on cols when orders identical
  for (i = 0; i < smi.j-1; i++) { // col counter
			 if (smi.orderWeights[i] == smi.orderWeights[i+1] &&
				 smi.orderColours[i] == smi.orderColours[i+1]) {
			   
			   vector<Var> orderMCol ;
			   vector<Var> orderMCol2 ;
			   for (i2 = 0; i2 < smi.j; i2++) {
				 orderMCol.push_back(Var(Bool, i2*smi.j+i)) ;
				 orderMCol2.push_back(Var(Bool, i2*smi.j+i+1)) ;
			   }
			   CSP.constraint(LexLeq, orderMCol, orderMCol2);
			   //add_constraint(LexLeqCon(orderMCol2, orderMCol)) ;
			 }
  }
  
  
  //  smi.colourM = vector<BoolVarRef>(smi.k * smi.j);
  
  
  for (i = 0; i < smi.j; i++) {
			 vector<Var> colourMRow;
			 for (i2 = 0; i2 < smi.k; i2++)
			   colourMRow.push_back(Var(Bool, start_of_colour_matrix + i*smi.k+i2)) ;
			 CSP.constraint(SumLeq, colourMRow, Var(Constant, 2));
			 CSP.constraint(SumGeq, colourMRow, Var(Constant, 2));
			 //	add_constraint(BoolLessEqualSumCon(colourMRow, compiletime_val<2>())) ;
			 //    add_constraint(BoolGreaterEqualSumCon(colourMRow, compiletime_val<2>()));
			 
  }
  
  //cout << "Colour matrix created" << endl ;
  
  
  for (i = 0; i < smi.j; i++)
			 for(i2 = 0; i2 < smi.j; i2++)
			   CSP.constraint(Ineq,Var(Bool, start_of_colour_matrix + i*smi.k+smi.orderColours[i2]-1), Var(Bool, i*smi.j + i2), Var(Constant,0));
  //      add_constraint(ImpliesCon(smi.OA[i*smi.j+i2], smi.colourM[i*smi.k+smi.orderColours[i2]-1])) ;
  
  // cout << "Channelling constraints created" << endl ;
  
  
  //BoundVarRef wasteage = boundvar_container.get_new_var(0, smi.getMaxWastage());
  /*
			
			add_to_var_order(smi.S);
			for(i=0;i<smi.j;i++)
			for(i2=0;i2<smi.j;i2++)
			add_to_var_order(smi.OA[i2*smi.j+i]);
			//  push_back_vector(var_order, smi.OA);
			//  push_back_vector(var_order, smi.colourM);
			
			vector<char> val_order_choose_min;
			for(i = 0; i < smi.S.size(); ++i)
			val_order_choose_min.push_back(1);
			for(i = 0; i < smi.OA.size(); ++i)
			val_order_choose_min.push_back(0);
			Controller::solve(val_order_choose_min);
			
			cerr << "FINISHED:" << Controller::solutions << " " << Controller::nodes << " " << Controller::current_optimise_position - 1 << endl;
			Controller::finish();*/
}
