/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: constraint_alldiff_gac_slow.h 668 2007-09-26 15:14:50Z pete_n $
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

#include <stdlib.h>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <utility>

using namespace std;

// ----------------------------- vertex-related stuff --------------------------------------

typedef enum {var, val, special} vertextype;

class vertex
{
public:
    inline friend bool operator==(const vertex& a, const vertex& b);
    inline friend bool operator<(const vertex& a, const vertex& b);
    vertex(vertextype a, long b) { kind=a; number=b;};
    void print(void) { cout<<"vertex:"<<(kind==var?"var":(kind==val?"val":"special"))<<":"<<number<<endl; };
private:
    vertextype kind;
    long number;
};

inline bool operator==(const vertex& a, const vertex& b)
{
    if(a.kind==b.kind)
    {
        if(a.number==b.number)
            return true;
        else
            return false;
    }
    else
        return false;
}

inline bool operator<(const vertex& a, const vertex& b)
{
    if(a.kind<b.kind)
        return true;
    else
    {
        if(a.kind>b.kind)
            return false;
        if(a.number<b.number)
            return true;
        else
            return false;
    }
}

class vertexlt {
public:
inline bool operator()(const vertex& a, const vertex& b) const
{
    return a<b;
}
};


// ---------------------------------- prototypes ------------------------------------------

bool recurse(long val, 
map<long, long, less<long> >& uprevious, 
set<long, less<long> >& uprevious_flag, 
map<long, vector<long>, less<long> >& vprevious, 
map<long, long, less<long> >& matching,
set<long, less<long> >& unmatched
);

bool hopcroft(unsigned int numvars, 
map<long, long>& matching, 
const set<long>& values);

void visit(vertex p, 
vector<vertex>& L, 
long& N, 
map<vertex, long, vertexlt>& dfsnum,
map<vertex, long, vertexlt>& low,
multimap<vertex, vertex, vertexlt>& edges,   //can't be const for some stupid reason.
vector<set<vertex, vertexlt> >& components,
set<vertex> & visited
);

template<typename VarArray>
struct AlldiffGacSlow : public Constraint
{
  virtual string constraint_name()
  { return "AlldiffGacSlow"; }
  
  typedef typename VarArray::value_type VarRef;  // what for?
  
  VarArray var_array;
  
  AlldiffGacSlow(StateObj* _stateObj, const VarArray& _var_array) : Constraint(_stateObj),
    var_array(_var_array)
  { }
  
  virtual triggerCollection setup_internal()
  {
    D_INFO(2,DI_SUMCON,"Setting up Constraint");
    triggerCollection t;
    int array_size = var_array.size();
    for(int i = 0; i < array_size; ++i)
	  t.push_back(make_trigger(var_array[i], Trigger(this, i), DomainChanged));
    return t;
  }
  
  virtual Constraint* reverse_constraint()
  { return new CheckAssignConstraint<VarArray, AlldiffGacSlow>(stateObj, var_array, *this); }
  
  PROPAGATE_FUNCTION(int prop_val, DomainDelta)
  {
	PROP_INFO_ADDONE(AlldiffGacSlow);
    
    unsigned int numvars=var_array.size();  // number of variables in the constraint
    
    map<long, long, less<long> > matching;   //maps value to variable number
    map<long, long>::iterator mapit;
    
    long temp;
    
    vector<long>::iterator vecit;
    
    set<long, less<long> > values;
    set<long, less<long> >::iterator setit;
    
    int res;
    unsigned int i, j;
    long tempval;
    
    // The Hopcroft-Karp algorithm for maximal bipartite matching.
    // Nodes are variables 0..(numvars-1) and domains (listed in listdomains)
    
    // matching[] maps values to variables 
    
    // Do greedy matching
    
    for(i=0; i<numvars; i++)
    {
        for(j=var_array[i].getMin(); j<=var_array[i].getMax(); j++)
        {
            if(var_array[i].inDomain(j))
            {
                mapit = matching.find(j);
                if ( mapit == matching.end() ) 
                {
                    matching[j]=i;
                    //cout<<"Variable:"<<i<<" Value:"<<tempval<<endl;
                    break;
                }
            }
        }
    }
    
    // Check length of the matching against the number of variables
    
    if(matching.size()!=(unsigned int)numvars)
    {
      //cout<<"Calling full search"<<endl;
      // If we need to, do the full Hopcroft-Karp search.
      if(!hopcroft(numvars, matching, values))
      {
        // The constraint is unsatisfiable (no matching).
        //cout<<"Fail."<<endl;
        getState(stateObj).setFailed(true);
        return;
      }
    }
    
    // Now we have a matching.
    
    /*cout <<"Success! Final matching:"<<endl;
    for(mapit=matching.begin(); mapit!=matching.end(); mapit++)
    {
        cout<<(*mapit).first<<":"<<(*mapit).second<<endl;
    }*/
    
    // Graph construction
    
    multimap <vertex, vertex, vertexlt> edges;
    multimap <vertex, vertex, vertexlt>::iterator edgesit;
    
    int tempvar;
    
    for(tempvar=0; tempvar<numvars; tempvar++)
    {
        for(DomainInt tempval=var_array[tempvar].getMin(); tempval<=var_array[tempvar].getMax(); tempval++)
        {
            if(var_array[tempvar].inDomain(tempval))
            {
                mapit=matching.find(tempval);
                if(mapit==matching.end() || tempvar!=matching[tempval])
                {
                    edges.insert(pair<vertex, vertex>(vertex(val, tempval),vertex(var, tempvar)));
                }
            }
        }
    }
    
    /*cout<<"Leftward edges:"<<endl;
    
    for(edgesit=edges.begin(); edgesit!=edges.end(); edgesit++)
    {
        cout<<"First:";
        ((vertex)(*edgesit).first).print();  // cast because it is a const
        cout<<"Second:";
        (*edgesit).second.print();
    }*/
    
    for(mapit=matching.begin(); mapit!=matching.end(); mapit++)
    {
        edges.insert(pair<vertex, vertex>(vertex(var, (*mapit).second), vertex(val, (*mapit).first)));
    }
    
    /*cout<<"Leftward and rightward edges:"<<endl;
    
    for(edgesit=edges.begin(); edgesit!=edges.end(); edgesit++)
    {
        cout<<"First:";
        ((vertex)(*edgesit).first).print();  // cast because it is a const
        cout<<"Second:";
        (*edgesit).second.print();
    }*/
    
    // add sink node - special 1 
    // edge from every used value to sink
    // edge from sink to every unused value.
    
    // set up values.
    for(int tempvar=0; tempvar<numvars; tempvar++)
    {
        for(DomainInt tempval=var_array[tempvar].getMin(); tempval<=var_array[tempvar].getMax(); tempval++)
        {
            if(var_array[tempvar].inDomain(tempval)) values.insert(tempval);
        }
    }
    
    for(setit=values.begin(); setit!=values.end(); setit++)
    {
        tempval=(*setit);
        if(matching.find(tempval)!=matching.end())
        {
            edges.insert(pair<vertex, vertex>(vertex(val, tempval), vertex(special, 1)));
        }
        else
        {
            edges.insert(pair<vertex, vertex>(vertex(special, 1), vertex(val, tempval)));
        }
    }
    
    /*cout<<"Leftward and rightward edges and sink node:"<<endl;
    
    for(edgesit=edges.begin(); edgesit!=edges.end(); edgesit++)
    {
        cout<<"First:";
        ((vertex)(*edgesit).first).print();  // cast because it is a const
        cout<<"Second:";
        (*edgesit).second.print();
    }*/
    
    // add node with an edge to all others - special 2 - Incorrect! Leave out special 2.
    // This is just for Tarjan's algorithm to use.
    
    for(setit=values.begin(); setit!=values.end(); setit++)
    {
        edges.insert(pair<vertex, vertex>(vertex(special, 2), vertex(val, (*setit))));
    }
    
    for(i=0; i<numvars; i++)
    {
        edges.insert(pair<vertex, vertex>(vertex(special, 2), vertex(var, i)));
    }
    
    edges.insert(pair<vertex, vertex>(vertex(special, 2), vertex(special, 1)));
    
    /*cout<<"Final graph"<<endl;
    
    for(edgesit=edges.begin(); edgesit!=edges.end(); edgesit++)
    {
        cout<<"First:";
        ((vertex)(*edgesit).first).print();  // cast because it is a const
        cout<<"Second:";
        (*edgesit).second.print();
    }*/
    
    // ------------------------------- call Targan's algorithm ----------------------------------
    /*
        Input: Graph G = (V, E), Start node v0

        max_dfs := 0  // Counter for dfs
        U := V        // Collection of unvisited nodes
        S := {}       // An initially empty stack
        tarjan(v0)    // Call the function with the start node
    */
    
    long N=0;
    
    vector<set<vertex, vertexlt> > components;
    
    vector<vertex> S;
    map<vertex, long, vertexlt> dfsnum;
    map<vertex, long, vertexlt> low;
    set<vertex> visited;
    
    visit(vertex(special, 2), S, N, dfsnum, low, edges, components, visited);
    
    vector<set<vertex, vertexlt> >::iterator compit;
    
    //  print components out
    
    /*cout<<"components:"<<endl;
    for(compit=components.begin(); compit!=components.end(); compit++)
    {
        set<vertex, vertexlt>::iterator cit;
        for(cit=(*compit).begin(); cit!=(*compit).end(); cit++)
        {
            ((vertex)(*cit)).print();
        }
        cout<<"Next component"<<endl;
    }*/
    
    // --------------------------- find residual edges to delete -------------------------------
    
    for(tempvar=0; tempvar<numvars; tempvar++)
    {
        for(DomainInt tempval=var_array[tempvar].getMin(); tempval<=var_array[tempvar].getMax(); tempval++)
        {
            if(var_array[tempvar].inDomain(tempval))
            {
                bool flag=true;  // tempval should be included in the domain
                
                if(matching.find(tempval)==matching.end() || tempvar!=matching[tempval])
                {
                    // tempval tempvar is a back edge - so not included unless 
                    // both tempvar and tempval are in the same component.
                    flag=false;
                    for(compit=components.begin(); compit!=components.end(); compit++)
                    {
                        if((*compit).find(vertex(var, tempvar))!=(*compit).end())
                        {
                            if((*compit).find(vertex(val, tempval))!=(*compit).end())
                            {
                                // tempval tempvar should be included in the domain
                                flag=true;
                            }
                            break;
                        }
                    }
                }
                // read flag here
                if(!flag)
                {
                    //cout<<"Adding "<<tempval<<endl;
                    var_array[tempvar].removeFromDomain(tempval);
                    if(getState(stateObj).isFailed()) return;
                }
            }
        }
    }
    
    return;
  }
  
  virtual BOOL full_check_unsat()
  { 
    int v_size = var_array.size();
    for(int i = 0; i < v_size; ++i)
	{
	  if(var_array[i].isAssigned())
	  {
	  
	    for(int j = i + 1; j < v_size; ++j)
		{
		  if(var_array[j].isAssigned())
		  {
		    if(var_array[i].getAssignedValue() == var_array[j].getAssignedValue())
		      return true;
		  }
		}
		
	  }
	}
	
	return false;
  }
  
  virtual BOOL check_unsat(int i, DomainDelta)
  {
    int v_size = var_array.size();
	D_ASSERT(var_array[i].isAssigned());
	DomainInt assign_val = var_array[i].getAssignedValue();
    for(int loop = 0; loop < v_size; ++loop)
	{
	  if(loop != i)
	  {
	    if(var_array[loop].isAssigned() && 
		   var_array[loop].getAssignedValue() == assign_val)
		return true;
	  }
	}
	return false;
  }
  
  virtual void full_propagate()
  {
    propagate(1, 0);
  }
	
	virtual BOOL check_assignment(vector<DomainInt> v)
	{
	  D_ASSERT(v.size() == var_array.size());
	  int array_size = v.size();
	  for(int i=0;i<array_size;i++)
		for( int j=i+1;j<array_size;j++)
		  if(v[i]==v[j]) return false;
	  return true;
	}
	
	virtual vector<AnyVarRef> get_vars()
	{
	  vector<AnyVarRef> vars;
	  vars.reserve(var_array.size());
	  for(unsigned i = 0; i < var_array.size(); ++i)
	    vars.push_back(var_array[i]);
	  return vars;
	}
    
    // ------------------------------- Targan's algorithm ------------------------------------
    
    void visit(vertex p, 
    vector<vertex>& S, 
    long& N,
    map<vertex, long, vertexlt>& dfsnum,
    map<vertex, long, vertexlt>& low,
    multimap<vertex, vertex, vertexlt>& edges,   //can't be const for some stupid reason.
    vector<set<vertex, vertexlt> >& components,
    set<vertex>& visited
    )
    {
        // based on the following pseudocode from wikipedia.
        /*
        Input: Graph G = (V, E), Start node v0

        max_dfs := 0  // Counter for dfs
        U := V        // Collection of unvisited nodes
        S := {}       // An initially empty stack
        tarjan(v0)    // Call the function with the start node
        
        procedure tarjan(v)
        v.dfs := max_dfs;          // Set the depth index
        v.lowlink := max_dfs;      // v.lowlink <= v.dfs
        max_dfs := max_dfs + 1;    // Increment the counter
        S.push(v);                 // Place v on the stack
        U := U \ {v};              // Separate v from U
        forall (v, v') in E do     // Consider the neighboring nodes
          if (v' in U)
            tarjan(v');            // recursive call
            v.lowlink := min(v.lowlink, v'.lowlink);
          // Ask whether v' is on the stack 
          // by a clever constant time method
          // (for example, setting a flag on the node when it is pushed or popped) 
          elseif (v' in S)
            v.lowlink := min(v.lowlink, v'.dfs);
          end if
        end for
        if (v.lowlink = v.dfs)     // the root of a strongly connected component
          print "SZK:";
          repeat
            v' := S.pop;
            print v';
          until (v' = v);
        end if
        */
        
        S.push_back(p);
        dfsnum[p]=N;
        low[p]=N;
        N++;
        visited.insert(p);
        
        multimap<vertex, vertex, vertexlt>::iterator edgeit;
        multimap<vertex, vertex, vertexlt>::iterator edgeit2;
        
        // Iterate through unvisited vertices.
        edgeit2=edges.upper_bound(p);
        for(edgeit=edges.lower_bound(p); edgeit!=edgeit2 && edgeit!=edges.end(); edgeit++)
        {
            //cout <<"In loop."<<endl;
            //cout<<"Pair:"<<endl;
            //((vertex)(*edgeit).first).print();  // why sometimes get garbage from these prints?
            //(*edgeit).second.print();
            vertex q=(*edgeit).second;
            
            if(visited.find(q)==visited.end())  // if not visited
            {
                // now we have an unvisited vertex in q.
                
                //cout <<"Making recursive call."<<endl;
                visit(q, S, N, dfsnum, low, edges, components, visited);
                
                low[p]=((low[p]<low[q])?low[p]:low[q]);
            }
            else
            {
                vector<vertex>::iterator sit = find(S.begin(), S.end(), q);
                if(sit!=S.end())
                {
                    D_ASSERT( (*sit)==q );
                    low[p]=((low[p]<dfsnum[q])?low[p]:dfsnum[q]);
                }
            }
        }
        
        if(low[p]==dfsnum[p])
        {
            // we are at the root of a strongly connected component
            
            set<vertex, vertexlt> tempset;
            while(true)
            {
                vertex v=(* --(S.end()));  //get last element
                //v.print();
                S.pop_back();  // why does this not return the value we need?
                tempset.insert(v);
                
                // remove vertex v from the graph
                /*for(edgeit=edges.begin(); edgeit!=edges.end(); edgeit++)
                {
                    if((*edgeit).first==v || (*edgeit).second==v)
                    {
                        edges.erase(edgeit);
                    }
                }*/
                
                if(v==p)
                {
                    break;
                }
            }
            components.push_back(tempset);
        }
    }
    
    // ------------------------------- Maximal matching functions -------------------------------
    
    // only called in one place so inline
    
    inline bool hopcroft(unsigned int numvars, 
    map<long, long>& matching, 
    const set<long>& values)
    {
        long tempval;
        long tempvar;
        vector<long> tempvec;
        unsigned int i;
        
        set<long, less<long> > layer;
        
        map<long, long>::iterator mapit;
        set<long>::iterator setit;
        
        vector<long>::iterator vecit;
        
        // uprevious == pred
        // vprevious == preds
        
        // need sets u and v
        // u is easy, v is union of domains[0..numvar-1]
        
        while(1)
        {
            set<long, less<long> > unmatched;
            map<long, long, less<long> > uprevious;  // variable to value  -- pred
            
            map<long, vector<long>, less<long> > vprevious;   // value to variable list  -- preds
            
            set<long, less<long> > uprevious_flag;   //  those elements in uprevious with the flag value "unmatched"
            
            layer.clear();
            //layer.erase(layer.begin(), layer.end());
            
            for(i=0; i<numvars; i++)
            {
                layer.insert(i);
                uprevious_flag.insert(i);
            }
                
            // iterate through matching.
            for(mapit=matching.begin(); mapit!=matching.end(); mapit++)
            {
                //cout<<"matching first:"<<(*mapit).first<<endl;
                layer.erase((*mapit).second);
                uprevious_flag.erase((*mapit).second);
            }    
            
            /*cout<<"layer:"<<endl;
            for(i=0; i<numvars; i++)
                cout <<i<<": "<<(layer.find(i)==layer.end()?0:1)<<endl;*/
            
            // we have now calculated layer
            
            while(layer.size()!=0 && unmatched.size()==0)
            {
                map<long, vector<long>, less<long> > newlayer;
                map<long, vector<long>, less<long> >::iterator newlayerit;
                
                for(setit=layer.begin(); setit!=layer.end(); setit++)
                {
                    //cout<<"Layer item: "<<(*setit)<<endl;
                    tempvar=(*setit);
                    for(DomainInt tempval=var_array[tempvar].getMin(); tempval<=var_array[tempvar].getMax(); tempval++)
                    {
                        if(var_array[tempvar].inDomain(tempval))
                        {
                            if(vprevious.find(tempval)==vprevious.end())
                            {
                                //newLayer.setdefault(v,[]).append(u)
                                newlayerit=newlayer.find(tempval);
                                if(newlayerit==newlayer.end())
                                {
                                    vector<long> tempvector;
                                    newlayer[tempval]=tempvector;
                                }
                                newlayer[tempval].push_back(tempvar);
                            }
                        }
                    }
                }
                
                /*cout<<"newlayer:"<<endl;
                for(newlayerit=newlayer.begin(); newlayerit!=newlayer.end(); newlayerit++)
                {
                    cout<<(*newlayerit).first<<" : ";
                    tempvec=(*newlayerit).second;
                    for(vecit=tempvec.begin(); vecit!=tempvec.end(); vecit++)
                    {
                        cout<<(*vecit);
                    }
                    cout<<endl;
                }*/
                
                // blank layer
                layer.clear();
                
                for(newlayerit=newlayer.begin(); newlayerit!=newlayer.end(); newlayerit++)
                {
                    tempval=(*newlayerit).first;
                    vprevious.erase(tempval);
                    tempvec=(*newlayerit).second;
                    
                    for(vecit=tempvec.begin(); vecit!=tempvec.end(); vecit++)
                    {
                        vprevious[tempval].push_back(*vecit);
                    }
                    mapit=matching.find(tempval);
                    
                    if(mapit!=matching.end())
                    {
                        layer.insert((*mapit).second);
                        uprevious[(*mapit).second]=tempval;
                    }
                    else
                    {
                        //cout<<"inserting value into unmatched:"<<tempval<<endl;
                        unmatched.insert(tempval);
                    }
                }
            }
            
            /*cout<<"Matching:"<<endl;
            for(mapit=matching.begin(); mapit!=matching.end(); mapit++)
            {
                cout<<(*mapit).first<<":"<<(*mapit).second<<endl;
            }
            
            cout<<"Unmatched:"<<endl;
            for(setit=unmatched.begin(); setit!=unmatched.end(); setit++)
            {
                cout<<(*setit)<<endl;
            }
            
            cout<<"Pred(uprevious):"<<endl;
            for(mapit=uprevious.begin(); mapit!=uprevious.end(); mapit++)
            {
                cout<<(*mapit).first<<":"<<(*mapit).second<<endl;
            }
            
            cout<<"uprevious flag:"<<endl;
            for(setit=uprevious_flag.begin(); setit!=uprevious_flag.end(); setit++)
            {
                cout<<(*setit)<<endl;
            }
            
            map<long, vector<long>, less<long> >::iterator vprevit;
            cout<<"vprevious:"<<endl;
            for(vprevit=vprevious.begin(); vprevit!=vprevious.end(); vprevit++)
            {
                cout<<(*vprevit).first<<":";
                vector<long> tempvec;
                tempvec=(*vprevit).second;
                
                for(vecit=tempvec.begin(); vecit!=tempvec.end(); vecit++)
                {
                    cout<<(*vecit)<<" ";
                }
                cout<<endl;
            }*/
            
            // did we finish layering without finding any alternating paths?
            // we do not need to calculate unlayered here.
             
            if(unmatched.size()==0)
            {
                if(matching.size()==numvars)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
                
            for(setit=unmatched.begin(); setit!=unmatched.end(); setit++)
            {
                tempval=(*setit);
                //cout<<"unmatched value:"<<tempval<<endl;
                recurse(tempval, uprevious, uprevious_flag, vprevious, matching, unmatched);
            }
        }
        return false;
    }
    
    bool recurse(long val, 
    map<long, long, less<long> >& uprevious, 
    set<long, less<long> >& uprevious_flag, 
    map<long, vector<long>, less<long> >& vprevious, 
    map<long, long, less<long> >& matching,
    set<long, less<long> >& unmatched
    )
    {
        //cout << "In recurse, value:"<<val<<endl;
        set<long>::iterator setit;
        vector<long>::iterator vecit;
        map<long, long>::iterator mapit;
        
        map<long, vector<long>, less<long> >::iterator vprevit;
        vprevit=vprevious.find(val);
        if(vprevit!=vprevious.end())
        {
            vector<long> listvars;  //L
            listvars= (*vprevit).second;
            vprevious.erase(vprevit);
            
            for(vecit=listvars.begin(); vecit!=listvars.end(); vecit++)  //for u in L
            {
                long tempvar=(*vecit);
                //cout<<"Variable: "<<tempvar<<endl;
                if((mapit=uprevious.find(tempvar))!=uprevious.end())  // if u in pred:
                {
                    long tempval=(*mapit).second;  //pu
                    uprevious.erase(mapit);
                    if(recurse(tempval, uprevious, uprevious_flag, vprevious, matching, unmatched))
                    {
                        matching[val]=tempvar;
                        return true;
                    }
                }
                else if((setit=uprevious_flag.find(tempvar))!=uprevious_flag.end())  // if u in pred as a flag:
                {
                    uprevious_flag.erase(setit);
                    matching[val]=(tempvar);
                    return true;
                }
            }
        }
        return false;
    }

    
  };



template<typename VarArray>
Constraint*
AlldiffGacSlowCon(StateObj* stateObj, const VarArray& var_array)
{ return new AlldiffGacSlow<VarArray>(stateObj, var_array); }

BUILD_CONSTRAINT1(CT_ALLDIFF_GACSLOW, AlldiffGacSlowCon)


