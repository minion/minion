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

/** @help constraints;watchelement_one Description
 This constraint is identical to watchelement, except the vector
 is indexed from 1 rather than from 0.
 */

/** @help constraints;watchelement_one References
 See entry
 
 help constraints watchelement
 
 for details of watchelement which watchelement_one is based on.
 */

/** @help constraints;watchelement Description
 The constraint 
 
 watchelement(vec, i, e)
 
 specifies that, in any solution, vec[i] = e and i is in the range 
 [0 .. |vec|-1].
 */

/** @help constraints;watchelement Notes
 Enforces generalised arc consistency.
 */

/** @help constraints;watchelement References
 See entry
 
 help constraints element
 
 for details of an identical constraint that enforces a lower level of
 consistency.
 */

// The triggers in this constraint are set up as follows:
// If the length of the vector is L.

// The first 2 * Dom(Result) literals are, for some j
//   literal 2 * i : attached to assignment i to V[j]
//   literal 2 * i + 1 : attached to the assignment j in IndexVar 

// After this there are 2 * Dom(Index) literals are, for some j
// literal 2 * i : attached to j in V[i]
// literal 2 * i + 1 : attached to j in Result

#ifndef CONSTRAINT_GAC2DELEMENT_H_OSAFDJ
#define CONSTRAINT_GAC2DELEMENT_H_OSAFDJ

// for the reverse constraint.
#include "../constraints/constraint_equal.h"
#include "../dynamic_constraints/dynamic_new_or.h"
#include "../dynamic_constraints/dynamic_new_and.h"
#include "../dynamic_constraints/unary/dynamic_literal.h"
#include "../dynamic_constraints/unary/dynamic_notinrange.h"
#include "../memory_management/reversible_vals.h"

template<typename VarArray, typename IndexArray, typename VarRef>


//notelement constraint
struct GAC2DElementNOTConstraint : public AbstractConstraint
{
	virtual string constraint_name()
	{ return "GAC2dElementNOT"; }
	
	
	VarArray var_array;
	typedef typename IndexArray::value_type Indexvartype;
	typedef typename VarArray::value_type VarArrayMember;
	
	Indexvartype indexvar1;
	Indexvartype indexvar2;
	VarRef resultvar;
	SysInt rowlength;
	
	
	Reversible<bool> full_propagate_called;//variable shows whether full_propagate has already been called
	
	DomainInt var_array_min_val;
	DomainInt var_array_max_val;
	
	DomainInt initial_result_dom_min;
	DomainInt initial_result_dom_max;
	
	vector<DomainInt> current_support_index1_index2;
	vector<DomainInt> current_support_index1_result;
	vector<DomainInt> current_support_index2_index1;
	vector<DomainInt> current_support_index2_result;
	vector<DomainInt> current_support_result_i1;
	vector<DomainInt> current_support_result_i2;
	
	
	
	
	
	
	GAC2DElementNOTConstraint(StateObj* _stateObj, const VarArray& _var_array, const IndexArray& _indexvar, const VarRef& _resultvar, DomainInt _rowlength) :
	AbstractConstraint(_stateObj),  var_array(_var_array), indexvar1(_indexvar[0]), indexvar2(_indexvar[1]), resultvar(_resultvar),
	rowlength(checked_cast<SysInt>(_rowlength)),full_propagate_called(_stateObj, false),
	var_array_min_val(0), var_array_max_val(0) 
	{ 
		
		initial_result_dom_min = resultvar.getInitialMin();
		initial_result_dom_max = resultvar.getInitialMax();
		
	}
	
	
	SysInt dynamic_trigger_count()
	{
		//the number of triggers needed
		SysInt count =  var_array.size()* 3 + var_array.size()* 3 +
		checked_cast<SysInt>(initial_result_dom_max - initial_result_dom_min + 1) * 3
		+ 1 
		+ 1
		+ 1; 
		
		SysInt rdomainsize = checked_cast<SysInt>(initial_result_dom_max - initial_result_dom_min + 1);
		//setup arrays that will be holding supporting value numbers
		current_support_result_i2.resize(rdomainsize); 
		current_support_result_i1.resize(rdomainsize);
		current_support_index1_result.resize(rowlength);
		current_support_index1_index2.resize(rowlength);
		current_support_index2_result.resize(var_array.size()/rowlength);
		current_support_index2_index1.resize(var_array.size()/rowlength);
		
		
		
		return count;
	}
	
	void find_new_support_for_result(int j)
	{
	    DomainInt realj = j + initial_result_dom_min;
		
		
		if(!resultvar.inDomain(realj)){
			return;
		}
		
		//int array_size = var_array.size();
		
		// support is value of index
		//we are looking for new support so basically we take the max value from which to start looping
		DomainInt oldsupport = max(current_support_result_i1[j], indexvar1.getMin());  // old support probably just removed
		DomainInt oldsupport2 = max(current_support_result_i2[j], indexvar2.getMin());  // old support probably just removed
		DomainInt maxsupport = indexvar1.getMax();
		DomainInt maxsupport2 = indexvar2.getMax();
		DomainInt maxsupport3;	
		DomainInt support = oldsupport;
		DomainInt support2 = oldsupport2;
		DomainInt support3=0;
			//get memory location to store triggers
		DynamicTrigger* dt = dynamic_trigger_start();
		bool found = false;
		while(support <= maxsupport && !found) {
			while (support2 <= maxsupport2  && !found) { 
				support3 = var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMin();
				maxsupport3 =var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMax();
				if (indexvar1.inDomain_noBoundCheck(support) && indexvar2.inDomain_noBoundCheck(support2)){
					if (support3 != realj){
						found =true;
						break;
					}
					
					else if (maxsupport3 != realj)
					{
						support3 = maxsupport3;
						found =true;
						break;
					}
					
				}		
				if (found){
					break;
				}
				++support2;
			}
			if(support2 > maxsupport2)//wrapover to start searching from first element
			{ 
				support2 = indexvar2.getMin();
				DomainInt max_check2 = min(oldsupport2, maxsupport2 + 1);
				while(support2 < max_check2) { 
					support3 = var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMin();
					maxsupport3 =var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMax();
					if (indexvar1.inDomain_noBoundCheck(support) && indexvar2.inDomain_noBoundCheck(support2)){
						if (support3 != realj){
							found =true;
							break;
						}
						
						else if (maxsupport3 != realj)
						{
							support3 = maxsupport3;
							found =true;
							break;
						}
						
					}
					
					if (found){
						break;
					}
					
					++support2;
				}
			}
			if (found){
				break;
			}
			++support;
		}
		//wrapover to look from support from the beginning
		
		if(support > maxsupport)
		{ 
			support = indexvar1.getMin();
			support2=oldsupport2;
			DomainInt max_check = min(oldsupport, maxsupport + 1);
			found = false;
			while(support < max_check){				
				while (support2 <= maxsupport2 && !found) { 
					if (indexvar1.inDomain_noBoundCheck(support) && indexvar2.inDomain_noBoundCheck(support2)){
						support3 = var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMin();
						maxsupport3 =var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMax();
						
						if (support3 != realj){
							found =true;
							break;
						}
						else if (maxsupport3 != realj)
						{
							support3 = maxsupport3;
							found =true;
							break;
						}
					}
					++support2;
				}
				if(support2 > maxsupport2) //wrapover to start searching from first element
				{ 
					support2 = indexvar2.getMin();
					DomainInt max_check2 = min(oldsupport2, maxsupport2 + 1);
					while(support2 < max_check2) { 
						if (indexvar1.inDomain_noBoundCheck(support) && indexvar2.inDomain_noBoundCheck(support2)){
							support3 = var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMin();
							maxsupport3 =var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].getMax();
							
							if (support3 != realj){
								found =true;
								break;
							}
							else if (maxsupport3 != realj)
							{
								support3 = maxsupport3;
								found =true;
								break;
							}
						}
						
						++support2;
					}
				}
				if (found){
					break;
				}
				++support;
			}
			if((support == max_check) && (!found))
			{
				resultvar.removeFromDomain(realj);
				return;
				
			}
		}
		//add new literal triggers
		var_array[rowlength * checked_cast<int>(support2) + checked_cast<int>(support)].addDynamicTrigger(dt + 3*j, DomainRemoval, support3);
		indexvar1.addDynamicTrigger(dt + 3*j + 1, DomainRemoval, support);
		indexvar2.addDynamicTrigger(dt + 3*j + 1+1, DomainRemoval, support2);
		current_support_result_i1[j] = support;
		current_support_result_i2[j] = support2;
	}
	
	
	
	void find_new_support_for_index2(int i)
	{
		
		if(!indexvar2.inDomain(i))
			return;
		
		DomainInt resultvarmin = resultvar.getMin();
		DomainInt resultvarmax = resultvar.getMax();
		//get memory location to store triggers
		DynamicTrigger* dt = dynamic_trigger_start() +
		checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3) + var_array.size() * 3;		
		bool found = false;
		//if result var is already assigned
		if(resultvarmin == resultvarmax)
		{
			
			for (SysInt j=checked_cast<SysInt>(indexvar1.getMin());j<checked_cast<SysInt>(indexvar1.getMax()+1);j++)
			{
				
				if (indexvar1.inDomain(j))
				{	
					DomainInt support3;
					if (var_array[rowlength*i+j].getMin() != resultvarmin){
						support3 = var_array[rowlength*i+j].getMin();
						found =true;
					}
					else if (var_array[rowlength*i+j].getMax() !=  resultvarmin)
					{
						support3 = var_array[rowlength*i+j].getMax();
						found =true;	
					}
					
					if (found)
					{
						var_array[rowlength*i+j].addDynamicTrigger(dt + 3*i, DomainRemoval, support3);
						resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, resultvarmin);
						indexvar1.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval,j);
						current_support_index2_result[i] = resultvarmin;
						current_support_index2_index1[i] = j;
						return;
					}
					
				}
			}
			
			if (!found){	
				indexvar2.removeFromDomain(i);
				return;
			}
		}
		//get previous supporting values
		DomainInt maxsupport_result = resultvarmax;
		DomainInt support_result = resultvarmin;
		DomainInt oldsupport_index1 = max(current_support_index2_index1[i], indexvar1.getMin()); // old support probably just removed
		DomainInt maxsupport_index1 = indexvar1.getMax();
		DomainInt support_index1 = oldsupport_index1;	
		DomainInt maxsupport3;	
		DomainInt support3;		
		found = false;
		while (support_index1 <= maxsupport_index1 && !found){
			support3 = var_array[checked_cast<SysInt>(rowlength*i+support_index1)].getMin();
			maxsupport3 = var_array[checked_cast<SysInt>(rowlength*i+support_index1)].getMax();
			if (support3 != support_result){
				found =true;
				break;
			}
			else if (support3 != maxsupport_result){
				support_result = maxsupport_result;
				found =true;
				break;
			}
			else if (maxsupport3 != maxsupport_result)
			{
				support3 = maxsupport3;
				support_result = maxsupport_result;
				found =true;
				break;
			}
			else if (maxsupport3 != support_result)
			{
				support3 = maxsupport3;
				found =true;
				break;
			}
			
			++support_index1;
		}		
		if(support_index1 > maxsupport_index1) 
		{ 
			support_index1 = indexvar1.getMin();//wrapover to start searching from first element
			support_result = resultvarmin;			
			DomainInt max_checki = min(oldsupport_index1, maxsupport_index1 + 1);
			found =false;
			while (support_index1 < max_checki && !found){				
				support3 = var_array[checked_cast<SysInt>(rowlength*i+support_index1)].getMin();
				maxsupport3 = var_array[checked_cast<SysInt>(rowlength*i+support_index1)].getMax();
				if (support3 != support_result){
					found =true;
					break;
				}
				else if (support3 != maxsupport_result){
					support_result = maxsupport_result;
					found =true;
					break;
				}
				else if (maxsupport3 != maxsupport_result)
				{
					support3 = maxsupport3;
					support_result = maxsupport_result;
					found =true;
					break;
				}
				else if (maxsupport3 != support_result)
				{
					support3 = maxsupport3;
					found =true;
					break;
				}
				
				++support_index1;				
			}
			if(( support_index1 >= max_checki ) && (!found)  )   
			{
				indexvar2.removeFromDomain(i); 
				return;
			}
			
		}
		//add new supporting literal triggers
		var_array[checked_cast<SysInt>(rowlength*i+support_index1)].addDynamicTrigger(dt + 3*i, DomainRemoval, support3);
		resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, support_result);
		indexvar1.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval, support_index1);
		current_support_index2_index1[i] = support_index1;
		current_support_index2_result[i] = support_result;
	}
	
	
	void find_new_support_for_index(int i)
	{
		
		if(!indexvar1.inDomain(i))
			return;
		
		DomainInt resultvarmin = resultvar.getMin();
		DomainInt resultvarmax = resultvar.getMax();		
		DynamicTrigger* dt = dynamic_trigger_start() +
		checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3);		
		bool found = false;		
		//if result variable is already assigned 
		if(resultvarmin == resultvarmax)
		{		
			for (SysInt j=checked_cast<SysInt>(indexvar2.getMin());j<checked_cast<SysInt>(indexvar2.getMax()+1);j++){
				if(indexvar2.inDomain(j))
				{	
					DomainInt support3;
					if (var_array[checked_cast<SysInt>(rowlength*j+i)].getMin() != resultvarmin){
						support3 = var_array[rowlength*j+i].getMin();
						
						found =true;
					}
					else if (var_array[rowlength*j+i].getMax() !=  resultvarmin)
					{
						support3 = var_array[rowlength*j+i].getMax();
						found =true;	
					}
					
					if (found)
					{
						var_array[rowlength*j+i].addDynamicTrigger(dt + 3*i, DomainRemoval, support3);
						resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, resultvarmin);
						indexvar2.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval,j);
						current_support_index1_result[i] = resultvarmin;
						current_support_index1_index2[i] = j;
						return;
					}
					
				}
			}
			
			if (!found){
				indexvar1.removeFromDomain(i);
				return;
			}
		}
		
		//get previous supporting values
		DomainInt maxsupport_result = resultvarmax;
		DomainInt support_result = resultvarmin;	
		DomainInt oldsupport_index2 = max(current_support_index1_index2[i], indexvar2.getMin()); // old support probably just removed
		DomainInt maxsupport_index2 = indexvar2.getMax();
		DomainInt support_index2 = oldsupport_index2;
		DomainInt maxsupport3;
		DomainInt support3;

		//int support = initial_result_dom_min;
		found = false;
		while (support_index2 <= maxsupport_index2 && !found){			
			support3 =  var_array[checked_cast<SysInt>(rowlength*support_index2+i)].getMin();
			maxsupport3 = var_array[checked_cast<SysInt>(rowlength*support_index2+i)].getMax();
			if (support3 != support_result){ //check against bounds
				found =true;
				break;
			}
			else if (support3 != maxsupport_result){
				support_result = maxsupport_result;
				found =true;
				break;
			}
			else if (maxsupport3 != maxsupport_result)
			{
				support3 = maxsupport3;
				support_result = maxsupport_result;
				found =true;
				break;
			}
			else if (maxsupport3 != support_result)
			{
				support3 = maxsupport3;
				found =true;
				break;
			}
			++support_index2;
		}
		
		if(support_index2 > maxsupport_index2) //wrapover to start searching from first element
		{ 			
			support_index2 = indexvar2.getMin();
			support_result = resultvarmin;
			DomainInt max_checki = min(oldsupport_index2, maxsupport_index2 + 1);
			found =false;
			while (support_index2 < max_checki && !found){
				support3 =  var_array[checked_cast<SysInt>(rowlength*support_index2+i)].getMin();
				maxsupport3 = var_array[checked_cast<SysInt>(rowlength*support_index2+i)].getMax();
				if (support3 != support_result){ //check against bounds
					found =true;
					break;
				}
				else if (support3 != maxsupport_result){
					support_result = maxsupport_result;
					found =true;
					break;
				}
				else if (maxsupport3 != maxsupport_result)
				{
					support3 = maxsupport3;
					support_result = maxsupport_result;
					found =true;
					break;
				}
				else if (maxsupport3 != support_result)
				{
					support3 = maxsupport3;
					found =true;
					break;
				}
				++support_index2;
				
			}			
			if(( support_index2 >= max_checki )  && (!found))    
			{
				indexvar1.removeFromDomain(i); 
				return;
			}
			
		}
		//add new supporting literal triggers		
		var_array[checked_cast<SysInt>(rowlength*support_index2+i)].addDynamicTrigger(dt + 3*i, DomainRemoval, support3);
		resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, support_result);
		indexvar2.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval, support_index2);
		current_support_index1_index2[i] = support_index2;
		current_support_index1_result[i] = support_result;
	}
	
	
	
	
	void deal_with_assigned_index()
	{
		D_ASSERT(indexvar1.isAssigned());
		D_ASSERT(indexvar2.isAssigned());
		int indexval1 = checked_cast<int>(indexvar1.getAssignedValue());
		int indexval2 = checked_cast<int>(indexvar2.getAssignedValue());
		if ((indexvar1.getMin() < 0) || (indexvar2.getMin() < 0) || (indexvar2.getMax() > ((SysInt)var_array.size()/rowlength)-1) || (indexvar1.getMax() > rowlength - 1)){
			return;
		}
		VarArrayMember var = var_array[checked_cast<SysInt>(rowlength*indexval2+indexval1)];
		
		//IF Z is assigned delete values from X that are also in Z
		if (resultvar.isAssigned()){
			for(DomainInt i = var.getMin(); i <= var.getMax(); ++i)
			{
				if((var.inDomain(i))){
					if (i == resultvar.getMin()){
						var.removeFromDomain(i); 
					}		
				}
			}
		}
		
		//IF X is assigned delete values from Z that are also in X
		if (var.getMin() == var.getMax()){
			for(DomainInt i = resultvar.getMin(); i <= resultvar.getMax(); ++i)
			{		
				if((resultvar.inDomain(i))){										
					if (i == var.getMin()){
						
						resultvar.removeFromDomain(i); 
					}
				}
			}
		}
	
	}
	
	
	bool find_out_of_range(){
		DynamicTrigger* dt = dynamic_trigger_start();
		dt +=  var_array.size()* 3 + var_array.size() *3  + checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3) ;
		++dt;
		++dt;
		//check for out of range value in Y1/Y2 and setup static trigger
		if (indexvar1.getMin() < 0){
			indexvar1.addDynamicTrigger(dt, DomainChanged);
			return true;
		}
		else if (indexvar1.getMax() > rowlength - 1){
			indexvar1.addDynamicTrigger(dt, DomainChanged);
			return true;
		}
		else if  (indexvar2.getMin() < 0){
			indexvar2.addDynamicTrigger(dt, DomainChanged);
			return true;
		}
		else if (indexvar2.getMax() > ((SysInt)(var_array.size())/rowlength)-1){
			indexvar2.addDynamicTrigger(dt, DomainChanged);
			return true;
		}
		return false;
	}
	
	
	
	virtual void full_propagate()
	{
		
		for(SysInt i=0; i<(SysInt)var_array.size(); i++) {
			if(var_array[i].isBound() && !var_array[i].isAssigned()) { // isassigned excludes constants.
				output_fatal_error("watchelement is not designed to be used on bound variables");
			}
		}
		if((indexvar1.isBound() && !indexvar1.isAssigned() ) ||(indexvar2.isBound() && !indexvar2.isAssigned())
		   || (resultvar.isBound() && !resultvar.isAssigned())) {
			output_fatal_error("watchelement is not designed to be used on bound variables");
		}
		
		
		if(!(find_out_of_range())){		
			full_propagate_called=true;
			
			DomainInt result_dom_size = initial_result_dom_max - initial_result_dom_min + 1;
			
			if(getState(stateObj).isFailed()) return;
			for(SysInt i = checked_cast<SysInt>(indexvar1.getMin()); i < checked_cast<SysInt>(indexvar1.getMax()+1); ++i)
			{
				current_support_index1_result[i] = initial_result_dom_min-1; 
				current_support_index1_index2[i] = -1; 
				
				
				if (indexvar1.inDomain(i)){
					// will be incremented if support sought
					find_new_support_for_index(i);
				}
			}
			
			for(SysInt i = checked_cast<SysInt>(indexvar2.getMin()); i < checked_cast<SysInt>(indexvar2.getMax()+1); ++i)
			{
				current_support_index2_result[i] = initial_result_dom_min-1;        // will be incremented if support sought
				current_support_index2_index1[i] = -1; 
				if (indexvar2.inDomain(i)){
					find_new_support_for_index2(i);
				}
			}
			
			
			for(int i = 0; i < result_dom_size; ++i)
			{
				current_support_result_i1[i] = -1; 
				current_support_result_i2[i] = -1; // will be incremented if support sought
				if(resultvar.inDomain(i + initial_result_dom_min))
					find_new_support_for_result(i);
			}
			if(indexvar1.isAssigned() && indexvar2.isAssigned())
			{
				deal_with_assigned_index();
			}
			DynamicTrigger* dt = dynamic_trigger_start();
			dt +=  var_array.size()* 3 + var_array.size() *3  + checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3) ;
			
			indexvar1.addDynamicTrigger(dt, Assigned);
			++dt;		
			indexvar2.addDynamicTrigger(dt, Assigned);
		}
	}
	
	
	virtual void propagate(DynamicTrigger* trig)
	{
		
		
		PROP_INFO_ADDONE(DynElement);
		DynamicTrigger* dt = dynamic_trigger_start();
		
		unsigned pos = trig - dt;
		unsigned array_size = var_array.size();
		unsigned result_support_triggers = 
		checked_cast<unsigned int>((initial_result_dom_max - initial_result_dom_min + 1) * 3);
		unsigned index_support_triggers =  array_size * 3 ;
		unsigned index_support_triggers2 =  var_array.size()* 3;

		if(find_out_of_range()){
			return;
		}
		
		if(pos < result_support_triggers)
		{// It was a value in the result var which lost support
			
			find_new_support_for_result(pos / 3);
			return;
		}
		pos -= result_support_triggers;
		
		if(pos < index_support_triggers)
		{// A value in the index var lost support
			
			
			find_new_support_for_index( pos / 3 );
			return;
		}
		pos -= index_support_triggers;
		
		if(pos < index_support_triggers2)
		{// A value in the index var lost support
			
			
			find_new_support_for_index2( pos / 3 );
			return;
		}
		pos -= index_support_triggers2;
		
		
		if (pos==0){
			
			// index1 has become assigned.
			if(indexvar1.isAssigned() && indexvar2.isAssigned())
			{				
				deal_with_assigned_index();
			}
			
			return;
		}
	
		if (pos==1){
			// index2 has become assigned.
			if(indexvar1.isAssigned() && indexvar2.isAssigned())
			{
				
				deal_with_assigned_index();
			}
					
			return;
		}
		D_ASSERT(pos == 2);
		if (pos==2){			
			// out of range trigger.
			if(!(find_out_of_range()))
			{				
				if (!(full_propagate_called)){
					full_propagate();
				}
			}
			
			return;
		}
		
	}
	
	virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
	{
		//	D_ASSERT(v_size == var_array.size() + 2);	
		
		int length = v_size;
		if((v[length-2] < 0) || (v[length-3] < 0) || ((v[length-2])>((SysInt)var_array.size()/rowlength)-1) || ((v[length-3])>rowlength-1)||
		   (v[length-2] > length - 3)){
			return true;
		}
		int i1 =checked_cast<int>(v[length-3]);
		int i2 =checked_cast<int>(v[length-2]);
		int tmp =rowlength * i2 + i1;
		//	bool ans = v[tmp] == v[length-1];
		
		//	int t = v[length-1];
		//	int t2 = v[tmp];
		
		return v[tmp] != v[length-1];
	}
	
	virtual vector<AnyVarRef> get_vars()
	{ 
		vector<AnyVarRef> array;
		array.reserve(var_array.size() + 2);
		for(unsigned int i=0;i<var_array.size(); ++i)
			array.push_back(var_array[i]);
		array.push_back(indexvar1);
		array.push_back(indexvar2);
		
		array.push_back(resultvar);
		return array;
	}
	
	
	virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
	{  
		SysInt array_start = 0;
		SysInt array_end   = SysInt(var_array.size()) - 1;
		DomainInt indexvar2_start = indexvar2.getMin();
		DomainInt indexvar2_end   = indexvar2.getMax();	
		DomainInt indexvar1_start = indexvar1.getMin();
		DomainInt indexvar1_end   = indexvar1.getMax();
		DomainInt res_start = resultvar.getMin();
		DomainInt res_end   = resultvar.getMax();
		
		// if index1 has an out of range value
		if ((indexvar1_end> rowlength-1)||(indexvar1_start<0)) {
			for(DomainInt i1 = indexvar1_start;i1 <= indexvar1_end; ++i1){
				if (indexvar1.inDomain(i1) && ((i1> rowlength-1)||(i1<0))){		
					for(SysInt i = array_start; i <= array_end; ++i)
					{
						for(DomainInt i2 = indexvar2_start;i2 <= indexvar2_end; ++i2)
						{
							if (indexvar2.inDomain(i2)){
								for(DomainInt resval = res_start; resval <= res_end; ++resval)
								{
									if (resultvar.inDomain(resval))
									{
										DomainInt dom_start = var_array[i].getMin();
										DomainInt dom_end   = var_array[i].getMax();
										for(DomainInt domval = dom_start; domval <= dom_end; ++domval)
										{
											// indexvar1 = i
											assignment.push_back(make_pair(var_array.size(), i1));
											// indexvar2 = i
											assignment.push_back(make_pair(var_array.size()+1, i2));
											// resultvar = domval
											assignment.push_back(make_pair(var_array.size() + 2, resval));
											// vararray[i] = domval
											assignment.push_back(make_pair(i, domval));
											return true;
											
										}
										
									}
								}
							}
						}
					}
				}
				
			}
		}
		// if index1 has an out of range value
		else if ((indexvar2_end> ((SysInt)var_array.size()/rowlength)-1)||(indexvar2_start<0)) {
			for(DomainInt i2 = indexvar2_start;i2 <= indexvar2_end; ++i2)
			{
				if (indexvar2.inDomain(i2) && ( (i2> ((SysInt)var_array.size()/rowlength)-1) || (i2<0)) )
				{		  
					for(int i = array_start; i <= array_end; ++i)
					{
						DomainInt indexvar1_start = indexvar1.getMin();
						DomainInt indexvar1_end   = indexvar1.getMax();
						for(DomainInt i1 = indexvar1_start;i1 <= indexvar1_end; ++i1)
						{						
							if (indexvar1.inDomain(i1)){
								DomainInt res_start = resultvar.getMin();
								DomainInt res_end   = resultvar.getMax();
								for(DomainInt resval = res_start; resval <= res_end; ++resval)
								{
									if (resultvar.inDomain(resval))
									{
										DomainInt dom_start =var_array[i].getMin();
										DomainInt dom_end   = var_array[i].getMax();
										for(DomainInt domval = dom_start; domval <= dom_end; ++domval)
										{	
											// indexvar1 = i
											assignment.push_back(make_pair(var_array.size(), i1));
											// indexvar2 = i
											assignment.push_back(make_pair(var_array.size()+1, i2));
											// resultvar = domval
											assignment.push_back(make_pair(var_array.size() + 2, resval));
											// vararray[i] = domval
											assignment.push_back(make_pair(i, domval));
											return true;
										}
										
									}
								}
							}
						}
					}
				}
			}
		}
		else {
			for(int i = array_start; i <= array_end; ++i)
			{
				int rowno = i / rowlength;
				int columnno = i % rowlength;		
				if ((indexvar1.inDomain(columnno)) && (indexvar2.inDomain(rowno)))
				{				
					DomainInt res_start = resultvar.getMin();
					DomainInt res_end   = resultvar.getMax();
					for(DomainInt resval = res_start; resval <= res_end; ++resval)
					{
						if (resultvar.inDomain(resval))
						{
							DomainInt dom_start =var_array[i].getMin();
							DomainInt dom_end   = var_array[i].getMax();
							for(DomainInt domval = dom_start; domval <= dom_end; ++domval)
							{
								
								if(var_array[i].inDomain(domval) && domval !=resval) {
									// indexvar1 = i
									assignment.push_back(make_pair(var_array.size(), columnno));
									// indexvar2 = i
									assignment.push_back(make_pair(var_array.size()+1, rowno));
									// resultvar = domval
									assignment.push_back(make_pair(var_array.size() + 2, resval));
									// vararray[i] = domval
									assignment.push_back(make_pair(i, domval));
									return true;
								}
							}
							
						}
					}
				}
			}
		}
		return false;
	}	
	
};

template<typename VarArray, typename IndexArray, typename VarRef>

struct GAC2DElementConstraint : public AbstractConstraint
{
	virtual string constraint_name()
	{ return "GAC2dElement"; }
	
	
	VarArray var_array;
	typedef typename VarArray::value_type VarArrayMember;
	typedef typename IndexArray::value_type Indexvartype;
	Indexvartype indexvar1;
	Indexvartype indexvar2;
	VarRef resultvar;
	int rowlength;
	
	
	DomainInt var_array_min_val;
	DomainInt var_array_max_val;
	
	DomainInt initial_result_dom_min;
	DomainInt initial_result_dom_max;
	
	vector<DomainInt> current_support_index1_index2;
	vector<DomainInt> current_support_index1_result;
	vector<DomainInt> current_support_index2_index1;
	vector<DomainInt> current_support_index2_result;
	vector<DomainInt> current_support_result_i1;
	vector<DomainInt> current_support_result_i2;
	
	
	
	//constructor
	GAC2DElementConstraint(StateObj* _stateObj, const VarArray& _var_array, const IndexArray& _indexvar, const VarRef& _resultvar, DomainInt _rowlength) :
	AbstractConstraint(_stateObj), var_array(_var_array), indexvar1(_indexvar[0]), indexvar2(_indexvar[1]), resultvar(_resultvar),
	rowlength(checked_cast<SysInt>(_rowlength)),
	var_array_min_val(0), var_array_max_val(0) 
	{ 
		initial_result_dom_min = resultvar.getInitialMin();
		initial_result_dom_max = resultvar.getInitialMax();
		
	}
	
	
	SysInt dynamic_trigger_count()
	{
		//the number of triggers to be used
		int count =  var_array.size()* 3 + var_array.size()* 3 +
		checked_cast<int>(initial_result_dom_max - initial_result_dom_min + 1) * 3
		+ 1 
		+ 1
		+ 1; 
		
		int rdomainsize = checked_cast<int>(initial_result_dom_max - initial_result_dom_min + 1);
		//setup arrays to hold supporting values
		current_support_result_i2.resize(rdomainsize);      
		current_support_result_i1.resize(rdomainsize);
		current_support_index1_result.resize(rowlength);
		current_support_index1_index2.resize(rowlength);
		current_support_index2_result.resize(var_array.size()/rowlength);
		current_support_index2_index1.resize(var_array.size()/rowlength);
		return count;
	}
	
	void find_new_support_for_result(int j)
	{
		DomainInt realj = j + initial_result_dom_min;
		
		if(!resultvar.inDomain(realj)){
			return;
		}
		
		
		//we are looking for new support so basically we take the value from which to start looping 
		//which is generally the previous supporting value
		DomainInt oldsupport = max(current_support_result_i1[j], indexvar1.getMin());  
		DomainInt oldsupport2 = max(current_support_result_i2[j], indexvar2.getMin());  
		DomainInt maxsupport = indexvar1.getMax();
		DomainInt maxsupport2 = indexvar2.getMax();
		
		DomainInt support = oldsupport;
		DomainInt support2 = oldsupport2;
		
		//intialise position where new support triggers will be stored
		DynamicTrigger* dt = dynamic_trigger_start();
		
		bool found = false;
		while(support <= maxsupport && !found) {
			support2 = oldsupport2;
			while (support2 <= maxsupport2) { 
				
				if (indexvar1.inDomain_noBoundCheck(support) && 
					indexvar2.inDomain_noBoundCheck(support2) &&
					var_array[checked_cast<SysInt>(rowlength * support2 + support)].inDomain(realj)){
					found = true;
					break;
				}
				++support2;
			}
			if(support2 > maxsupport2)// warpover to start seraching from the beginning
			{ 
				support2 = indexvar2.getMin();
				DomainInt max_check2 = min(oldsupport2, maxsupport2 + 1);
				while(support2 < max_check2) { 
					
					if (indexvar1.inDomain_noBoundCheck(support) && 
						indexvar2.inDomain_noBoundCheck(support2) &&
						var_array[checked_cast<SysInt>(rowlength * support2 + support)].inDomain(realj)){
						found = true;
						break;
					}
					++support2;
				}
			}
			if (found){
				break;
			}
			++support;
		}
		
		//wrapover to look from support from the beginning
		
		if(support > maxsupport)
		{ 
			support = indexvar1.getMin();
			support2=oldsupport2;
			DomainInt max_check = min(oldsupport, maxsupport + 1);
			found = false;
			while(support < max_check){
				support2 = oldsupport2;
				while (support2 <= maxsupport2 && !found) { 
					if (indexvar1.inDomain_noBoundCheck(support) && 
						indexvar2.inDomain_noBoundCheck(support2) &&
						var_array[checked_cast<SysInt>(rowlength * support2 + support)].inDomain(realj)){
						found = true;
						break;
					}
					++support2;
				}
				if(support2 > maxsupport2)
				{ 
					support2 = indexvar2.getMin();
					DomainInt max_check2 = min(oldsupport2, maxsupport2 + 1);
					while(support2 < max_check2) { 
						if (indexvar1.inDomain_noBoundCheck(support) && 
							indexvar2.inDomain_noBoundCheck(support2) &&
							var_array[checked_cast<SysInt>(rowlength * support2 + support)].inDomain(realj)){
							found = true;
							break;
						}
						++support2;
					}
					
				}
				if (found){
					break;
				}
				++support;
			}
			if((support == max_check) && (!found))
			{
				resultvar.removeFromDomain(realj);
				return;
				
			}
		}
		//add new literal triggers on new supporting values
		var_array[checked_cast<SysInt>(rowlength * support2 + support)].addDynamicTrigger(dt + 3*j, DomainRemoval, realj);
		indexvar1.addDynamicTrigger(dt + 3*j + 1, DomainRemoval, support);
		indexvar2.addDynamicTrigger(dt + 3*j + 1+1, DomainRemoval, support2);
		current_support_result_i1[j] = support;
		current_support_result_i2[j] = support2;
	}
	
	
	void find_new_support_for_index2(int i)
	{
		if(!indexvar2.inDomain(i))
			return;
		
		DomainInt resultvarmin = resultvar.getMin();
		DomainInt resultvarmax = resultvar.getMax();
		
		
		DynamicTrigger* dt = dynamic_trigger_start() +
		checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3) + var_array.size() * 3;		
		bool found = false;
		
		//if there is only one element in the domain of resultvar
		if(resultvarmin == resultvarmax)
		{
			for (SysInt j=checked_cast<SysInt>(indexvar1.getMin());j<checked_cast<SysInt>(indexvar1.getMax()+1);j++){
				if (indexvar1.inDomain(j) && var_array[rowlength*i+j].inDomain(resultvarmin)){
					var_array[rowlength*i+j].addDynamicTrigger(dt + 3*i, DomainRemoval, resultvarmin);
					resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, resultvarmin);
					indexvar1.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval,j);
					current_support_index2_result[i] = resultvarmin;
					current_support_index2_index1[i] = j;
					found=true;
					return;
				}
			}
			if (!found){
				indexvar2.removeFromDomain(i);
				return;
			}
			
		}
		
		//we are looking for new support so basically we take the value from which to start looping 
		//which is generally the previous supporting value
		DomainInt oldsupport_result = max(current_support_index2_result[i], resultvarmin); // old support probably just removed
		DomainInt maxsupport_result = resultvarmax;
		DomainInt support_result = oldsupport_result;
		
		DomainInt oldsupport_index1 = max(current_support_index2_index1[i], indexvar1.getMin()); // old support probably just removed
		DomainInt maxsupport_index1 = indexvar1.getMax();
		DomainInt support_index1 = oldsupport_index1;
		
		found = false;
		while (support_index1 <= maxsupport_index1 && !found){
			support_result = oldsupport_result;
			while(support_result <= maxsupport_result){
				if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*i+support_index1)].inDomain(support_result))){
					found =true;
					break;
				}
				++support_result;
			}	
			if(support_result > maxsupport_result)
			{ 
				support_result = resultvarmin;//wrapover to start searching from first element
				DomainInt max_check = min(oldsupport_result, maxsupport_result + 1);
				found = false;
				while(support_result < max_check && !found){    
					if((resultvar.inDomain_noBoundCheck(support_result)) && 
					   (var_array[checked_cast<SysInt>(rowlength*i+support_index1)].inDomain(support_result))){
						found = true; 
						break;						
					}
					++support_result;					
				}
			}
			if (found){
				break;
			}
			++support_index1;
		}		
		if(support_index1 > maxsupport_index1)
		{ 
			support_index1 = indexvar1.getMin();//wrapover to start searching from first element
			support_result = oldsupport_result;			
			DomainInt max_checki = min(oldsupport_index1, maxsupport_index1 + 1);
			found =false;
			while (support_index1 < max_checki && !found){			
				support_result = oldsupport_result;
				while(support_result <= maxsupport_result){
					if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*i+support_index1)].inDomain(support_result))){
						found = true;
						break;//stop loopong
					}
					++support_result;
				}				
				if(support_result > maxsupport_result)//wrapover to start searching from first element
				{ 
					support_result = resultvarmin;
					DomainInt max_check = min(oldsupport_result, maxsupport_result + 1);
					found= false;
					while(support_result < max_check){     
						if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*i+support_index1)].inDomain(support_result))){
							found = true;
							break;//stop loopong
						}
						++support_result;
					}					
				}
				if (found){
					break;//stop loopong
				}
				
				++support_index1;				
			}
			//prune element
			if(( support_index1 >= max_checki ) && (!found)  )   
			{
				indexvar2.removeFromDomain(i); 
				return;
			}
			
		}
		//add new support literals
		var_array[checked_cast<SysInt>(rowlength*i+support_index1)].addDynamicTrigger(dt + 3*i, DomainRemoval, support_result);
		resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, support_result);
		indexvar1.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval, support_index1);
		current_support_index2_index1[i] = support_index1;
		current_support_index2_result[i] = support_result;
	}
	
	
	void find_new_support_for_index(int i)
	{
		if(!indexvar1.inDomain(i))
			return;
		
		DomainInt resultvarmin = resultvar.getMin();
		DomainInt resultvarmax = resultvar.getMax();
		
		//find location to store triggers
		DynamicTrigger* dt = dynamic_trigger_start() +
		checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3);		
		bool found = false;
		
		//if there is only one value in the domain of resultvar
		if(resultvarmin == resultvarmax)
		{
			for (SysInt j=checked_cast<SysInt>(indexvar2.getMin());j<checked_cast<SysInt>(indexvar2.getMax()+1);j++){
				if(indexvar2.inDomain(j) && var_array[rowlength*j+i].inDomain(resultvarmin)){
					var_array[rowlength*j+i].addDynamicTrigger(dt + 3*i, DomainRemoval, resultvarmin);
					resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, resultvarmin);
					indexvar2.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval,j);
					current_support_index1_result[i] = resultvarmin;
					current_support_index1_index2[i] = j;
					found=true;
					return;
				}
				
			}	
			if (!found){
				indexvar1.removeFromDomain(i);
				return;
			}
		}
		
		
		//we are looking for new support so basically we take the value from which to start looping 
		//which is generally the previous supporting value
		DomainInt oldsupport_result = max(current_support_index1_result[i], resultvarmin); 
		DomainInt maxsupport_result = resultvarmax;
		DomainInt support_result = oldsupport_result;	
		
		DomainInt oldsupport_index2 = max(current_support_index1_index2[i], indexvar2.getMin()); 
		DomainInt maxsupport_index2 = indexvar2.getMax();
		DomainInt support_index2 = oldsupport_index2;
		
		
		
		//start searching for support from location of previous support
		found = false;
		while (support_index2 <= maxsupport_index2 && !found){			
			support_result = oldsupport_result;
			while(support_result <= maxsupport_result){
				if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*support_index2+i)].inDomain(support_result))){	
					found =true;
					break;
				}
				++support_result;
			}		
			if(support_result > maxsupport_result)//wrapover to start searching from first element
			{ 
				
				support_result = resultvarmin;
				DomainInt max_check = min(oldsupport_result, maxsupport_result + 1);
				found = false;
				while(support_result < max_check && !found){    
					if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*support_index2+i)].inDomain(support_result))){	
						found = true; 
						break;	//stop looping					
					}					
					++support_result;					
				}
			}
			if (found){
				break; //stop looping
			}
			++support_index2;
		}
		
		if(support_index2 > maxsupport_index2)
		{ 
			support_index2 = indexvar2.getMin();//wrapover to start searching from first element
			support_result = oldsupport_result;
			DomainInt max_checki = min(oldsupport_index2, maxsupport_index2 + 1);
			found =false;
			while (support_index2 < max_checki && !found){
				support_result = oldsupport_result;
				while(support_result <= maxsupport_result){
					if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*support_index2+i)].inDomain(support_result))){
						
						found = true;
						break;
					}
					++support_result;
					
				}
				if(support_result > maxsupport_result)//wrapover to start searching from first elementr
				{ 
					support_result = resultvarmin;
					DomainInt max_check = min(oldsupport_result, maxsupport_result + 1);
					found= false;
					while(support_result < max_check){     
						if((resultvar.inDomain_noBoundCheck(support_result)) && (var_array[checked_cast<SysInt>(rowlength*support_index2+i)].inDomain(support_result))){
							found = true;
							break;
						}
						++support_result;
					}
				}
				
				if (found){
					break;
				}
				++support_index2;
				
			}
			
			//prune element
			if(( support_index2 >= max_checki )  && (!found))    
			{
				indexvar1.removeFromDomain(i); 
				return;
			}
			
		}
		//add new support literals
		var_array[checked_cast<SysInt>(rowlength*support_index2+i)].addDynamicTrigger(dt + 3*i, DomainRemoval, support_result);
		resultvar.addDynamicTrigger(dt + 3*i + 1, DomainRemoval, support_result);
		indexvar2.addDynamicTrigger(dt + 3*i + 1+1, DomainRemoval, support_index2);
		current_support_index1_index2[i] = support_index2;
		current_support_index1_result[i] = support_result;
		
	}
	
	
	void deal_with_assigned_index()
	{
		D_ASSERT(indexvar1.isAssigned());
		D_ASSERT(indexvar2.isAssigned());
		int indexval1 = checked_cast<int>(indexvar1.getAssignedValue());
		int indexval2 = checked_cast<int>(indexvar2.getAssignedValue());
		
		VarArrayMember var = var_array[rowlength*indexval2+indexval1];
		
		DomainInt lower = resultvar.getMin(); 
		if( lower > var.getMin() ) 
		{
			var.setMin(lower);
			++lower;                      // do not need to check lower bound, we know it's in resultvar
		}
		
		DomainInt upper = resultvar.getMax(); 
		if( upper < var.getMax() ) 
		{
			var.setMax(upper);
			--upper;                      // do not need to check upper bound, we know it's in resultvar
		}
		
		for(DomainInt i = lower; i <= upper; ++i)
		{
			if(!(resultvar.inDomain(i)))
				var.removeFromDomain(i); 
		}
	}
	
	
	
	
	
	virtual void full_propagate()
	{
		for(SysInt i=0; i<(SysInt)var_array.size(); i++) {
			if(var_array[i].isBound() && !var_array[i].isAssigned()) { // isassigned excludes constants.
				cerr << "Warning: watchelement is not designed to be used on bound variables and may cause crashes." << endl;
			}
		}
		if((indexvar1.isBound() && !indexvar1.isAssigned() ) ||(indexvar2.isBound() && !indexvar2.isAssigned())
		   || (resultvar.isBound() && !resultvar.isAssigned())) {
			cerr << "Warning: watchelement is not designed to be used on bound variables and may cause crashes." << endl;
		}
		
		int array_size = var_array.size(); 
		
		
		DomainInt result_dom_size = initial_result_dom_max - initial_result_dom_min + 1;
		
		//set the legal bounds for indexvar1 and index var2
		indexvar1.setMin(0);
		indexvar1.setMax(rowlength - 1);
		
		indexvar2.setMin(0);
		indexvar2.setMax((array_size/rowlength)-1);
		
		if(getState(stateObj).isFailed()) return;
		
		for(SysInt i = checked_cast<SysInt>(indexvar1.getMin()); i < checked_cast<SysInt>(indexvar1.getMax()+1); ++i)
		{
			
			current_support_index1_result[i] = initial_result_dom_min-1; 
			current_support_index1_index2[i] = -1; 				// will be incremented if support sought
			
			
			if (indexvar1.inDomain(i)){
				find_new_support_for_index(i);
			}
		}
		
		for(SysInt i = checked_cast<SysInt>(indexvar2.getMin()); i < checked_cast<SysInt>(indexvar2.getMax()+1); ++i)
		{
			current_support_index2_result[i] = initial_result_dom_min-1; // will be incremented if support sought
			current_support_index2_index1[i] = -1; 
			if (indexvar2.inDomain(i)){
				find_new_support_for_index2(i);
			}
		}
		
		
		for(int i = 0; i < result_dom_size; ++i)
		{
			current_support_result_i1[i] = -1; 
			current_support_result_i2[i] = -1; // will be incremented if support sought
			if(resultvar.inDomain(i + initial_result_dom_min))
				find_new_support_for_result(i); 
		}
		
		if(indexvar1.isAssigned() && indexvar2.isAssigned())
			deal_with_assigned_index();
		
		
		DynamicTrigger* dt = dynamic_trigger_start();
		dt +=  var_array.size()* 3 + var_array.size() *3  + checked_cast<int>((initial_result_dom_max - initial_result_dom_min + 1) * 3) ;
		
		//static triggers
		resultvar.addDynamicTrigger(dt, DomainChanged);  
		++dt;
		indexvar1.addDynamicTrigger(dt, Assigned);
		++dt;		
		indexvar2.addDynamicTrigger(dt, Assigned);
	}
	
	
	virtual void propagate(DynamicTrigger* trig)
	{
		
		PROP_INFO_ADDONE(DynElement); //used for info purposes
		DynamicTrigger* dt = dynamic_trigger_start();
		unsigned pos = trig - dt;
		unsigned array_size = var_array.size();
		//there are 3 watched triggers for each literal, since three things must be supporting each literal.
		unsigned result_support_triggers = 
		checked_cast<unsigned int>((initial_result_dom_max - initial_result_dom_min + 1) * 3);
		unsigned index_support_triggers =  array_size * 3 ;
		unsigned index_support_triggers2 =  var_array.size()* 3;
		
		if(pos < result_support_triggers)
		{// It was a value in the result var which lost support
			find_new_support_for_result(pos / 3);
			return;
		}
		pos -= result_support_triggers;		
		if(pos < index_support_triggers)
		{// A value in the index var lost support
			
			find_new_support_for_index( pos / 3 );
			return;
		}
		pos -= index_support_triggers;
		
		if(pos < index_support_triggers2)
		{// A value in the index var 2 lost support
			find_new_support_for_index2( pos / 3 );
			return;
		}
		pos -= index_support_triggers2;
		
		if (pos == 0)
		{ // A value was removed from result var
			
			if(indexvar1.isAssigned() && indexvar2.isAssigned())
			{
				deal_with_assigned_index();
			}
			
			return;
		}		
		if (pos==1){
			
			// index1 has become assigned.
			if(indexvar1.isAssigned() && indexvar2.isAssigned())
			{
				deal_with_assigned_index();
				
			}
			return;
		}	
		D_ASSERT(pos == 2);
		
		if (pos==2){
			// index2 has become assigned.
			if(indexvar1.isAssigned() && indexvar2.isAssigned())
			{
				deal_with_assigned_index();
			}
			return;
		}
		
	}
	
	virtual BOOL check_assignment(DomainInt* v, SysInt v_size)
	{
		//checks that an assignement is valid
		int length = v_size;
		if(v[length-2] < 0 || (v[length-3] < 0) ||
		   v[length-2] > length - 3){
			return false;
		}
		int i1 =checked_cast<int>(v[length-3]);
		int i2 =checked_cast<int>(v[length-2]);
		int tmp =rowlength * i2 + i1;
		return v[tmp] == v[length-1];
	}
	
	virtual vector<AnyVarRef> get_vars()
	{ 
		//returns variables
		vector<AnyVarRef> array;
		array.reserve(var_array.size() + 2);
		for(unsigned int i=0;i<var_array.size(); ++i)
			array.push_back(var_array[i]);
		array.push_back(indexvar1);
		array.push_back(indexvar2);
		
		array.push_back(resultvar);
		return array;
	}
	
	virtual bool get_satisfying_assignment(box<pair<SysInt,DomainInt> >& assignment)
	{  
		SysInt array_start = 0;
		SysInt array_end   = SysInt(var_array.size()) - 1;
		
		for(int i = array_start; i <= array_end; ++i)
		{
			int rowno = i / rowlength;
			int columnno = i % rowlength;
			
			if ((indexvar1.inDomain(columnno)) && (indexvar2.inDomain(rowno)))
			{
				DomainInt dom_start = max(resultvar.getMin(), var_array[i].getMin());
				DomainInt dom_end   = min(resultvar.getMax(), var_array[i].getMax());
				for(DomainInt domval = dom_start; domval <= dom_end; ++domval)
				{
					if(var_array[i].inDomain(domval) && resultvar.inDomain(domval))
					{
						// indexvar1 = i
						assignment.push_back(make_pair(var_array.size(), columnno));
						// indexvar2 = i
						assignment.push_back(make_pair(var_array.size()+1, rowno));
						// resultvar = domval
						assignment.push_back(make_pair(var_array.size() + 2, domval));
						// vararray[i] = domval
						assignment.push_back(make_pair(i, domval));
						return true;
					}
				}
			}
		}
		return false;
	}
	
	
	virtual AbstractConstraint* reverse_constraint()
	{
		
		//need to repackage index back into an array
		IndexArray indexvar;
		indexvar.push_back(indexvar1);
		indexvar.push_back(indexvar2);
		//calls not element
		return new GAC2DElementNOTConstraint<VarArray,IndexArray, VarRef>(stateObj, var_array, indexvar, resultvar, rowlength);
		
	}
	
};

#endif
