/*
* Minion http://minion.sourceforge.net
* Copyright (C) 2006-13
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

#ifndef _HAGGIS_GAC_TUPLES_CHUDSCDFDSAFDSA
#define _HAGGIS_GAC_TUPLES_CHUDSCDFDSAFDSA

 struct SupportDeref
    {
        template<typename T>
        bool operator()(const T& lhs, const T& rhs)
        { return *lhs < *rhs; }
    };

struct HaggisGACTuples
{
    typedef vector<vector<vector<vector<pair<SysInt, DomainInt> > * > > > tuple_list_type;

    vector<pair<DomainInt, DomainInt> > doms;

    tuple_list_type tuple_list_cpy;

    template<typename Vars>
    bool verify_domains(const Vars& vars)
    {
        if(vars.size() != doms.size())
            return false;

        for(SysInt i = 0; i < doms.size(); ++i)
        {
            if(doms[i] != std::make_pair(vars[i].getInitialMin(), vars[i].getInitialMax()))
                return false;
        }
        return true;
    }

    const tuple_list_type& get_tl() const
    { return tuple_list_cpy; }

    template<typename Vars, typename Data>
    HaggisGACTuples(const Vars& vars, Data data)
    {
        for(SysInt i = 0; i < vars.size(); ++i)
            doms.push_back(std::make_pair(vars[i].getInitialMin(), vars[i].getInitialMax()));

        // Read in the short supports.
        vector<vector<pair<SysInt, DomainInt> > * > shortsupports;
        
        const vector<vector<pair<SysInt, DomainInt> > >& tupleRef = (*data->tuplePtr());
        
        for(SysInt i=0; i<tupleRef.size(); i++) {
            shortsupports.push_back(new vector<pair<SysInt, DomainInt> >(tupleRef[i]));
        }

        
        // Sort it. Might not work when it's pointers.
        for(SysInt i=0; i<shortsupports.size(); i++) {
            sort(shortsupports[i]->begin(), shortsupports[i]->end());
        }
        sort(shortsupports.begin(), shortsupports.end(), SupportDeref());
        
        tuple_list_cpy.resize(vars.size());
        for(SysInt var=0; var<vars.size(); var++) {
            SysInt domsize = checked_cast<SysInt>(vars[var].getInitialMax()-vars[var].getInitialMin()+1);
            tuple_list_cpy[var].resize(domsize);
            
            for(DomainInt val=vars[var].getInitialMin(); val<=vars[var].getInitialMax(); val++) {
                // get short supports relevant to var,val.
                for(SysInt i=0; i<shortsupports.size(); i++) {
                    bool varin=false;
                    bool valmatches=true;
                    
                    vector<pair<SysInt, DomainInt> > & shortsup=*(shortsupports[i]);
                    
                    for(SysInt j=0; j<shortsup.size(); j++) {
                        if(shortsup[j].first==var) {
                            varin=true;
                            if(shortsup[j].second!=val) {
                                valmatches=false;
                            }
                        }
                    }
                    
                    if(!varin || valmatches) {
                        // If the support doesn't include the var, or it 
                        // does include var,val then add it to the list.
                        tuple_list_cpy[var][checked_cast<SysInt>(val-vars[var].getInitialMin())].push_back(shortsupports[i]);
                    }
                }
            }
        }
    }
};

template<typename Vars>
inline HaggisGACTuples* ShortTupleList::getHaggisData(const Vars& vars)
{
  if(hgt == NULL)
    hgt = new HaggisGACTuples(vars, this);

  std::ostringstream oss;
  oss << "The short tuple '" + tuple_name + "' is used in two haggisgac constraints\n";
  oss << "With either different numbers of variables, or different domains.\n";
  oss << "This is currently not allowed, for a boring technical reason (sorry).\n";
  oss << "You could duplicate the short tuple for each different set of domains,\n";
  oss << "or, if you complain to the minion developers we promise to fix it!\n";
  CHECK(hgt->verify_domains(vars), oss.str());

  return hgt;
}

#endif
