#include <algorithm>
#include <vector>
#include <array>
#include <utility>
#include <cassert>
#include <string>
#include <iostream>
#include <ostream>
#include <math.h>


#include "constraints.hpp"
#include "data_point.hpp"
#include "propagation.hpp"

template<typename T, size_t s>
void clear(array<T,s>& a, T t = 0)
{
    for(size_t i = 0; i < s; ++i)
        a[i] = t;
}

template<typename C>
vector<array<int, vcount>> build_constraint(C c)
{
    array<int, vcount> data;
    clear(data);
    vector<array<int, vcount>> tuples;
    do
    {
        if(c(data))
            tuples.push_back(data);

    }while(increment_array(data, 2));
    
    return tuples;

}

template<typename C>
DataList build_datapoints(C c)
{
    DataList s;
    InData data;
    clear(data);

    do
    {
        bool valid_domains = true;
        for(int i = 0; i < vcount; ++i)
        {
            int domval_present = false;
            for(int j = 0; j < domsize; ++j)
            {
                if(data[i*domsize+j])
                    domval_present = true;
            }
            if(!domval_present)
                valid_domains = false;
        }
        if(valid_domains)
        {
            s.insertDP(DataPoint(data, prune_domains(data, c)));
        }
    }
    while(increment_array(data, 2));
    return s;
}


struct TreeVertex
{
    TreeVertex* true_b;
    TreeVertex* false_b;
    int branchval;
    InData* prop;
    bool fail_node;

    TreeVertex(TreeVertex* _lhs, TreeVertex* _rhs, int _b) :
    true_b(_lhs), false_b(_rhs), branchval(_b), prop(NULL), fail_node(false)
    { }

    TreeVertex(const InData& dc) :
    true_b(NULL), false_b(NULL), branchval(-1), prop(new InData(dc)), fail_node(false)
    { }

    static TreeVertex* FailTreeVertex()
    {
        TreeVertex* tv = new TreeVertex(NULL, NULL, -1);
        tv->fail_node = true;
        return tv;
    }

    bool isFinal() const
    {
        assert( (!true_b) == (!false_b) );
        return !true_b; 
    }

    bool isFinalTrivial() const
    {
        if(!isFinal()) return false;

        for(int i = 0; i < dcount; ++i)
            if(!(*prop)[i])
                return false;
        return true;

}

    int depth() const
    {
        int true_size =  true_b ?true_b->depth():0;
        int false_size = false_b?false_b->depth():0;
        return 1 + max(true_size, false_size);
    }

    int size() const
    {
        int true_size = 0, false_size = 0;
        if(true_b) true_size = true_b->size();
        if(false_b) false_size = false_b->size();
        return 1 + true_size + false_size;
    }

    ~TreeVertex()
    {
        delete true_b;
        delete false_b;
    }


    void print(string indent = "", vector<int> deleted_vals = vector<int>()) const
    {
        if(isFinal())
        {
            if(fail_node)
            {
                cout << indent << "FAIL();" << endl;
                return;
            }
            int count = 0;
            for(int i = 0; i < dcount; ++i)
                if(!(*prop)[i])
                    count++;
            
            if(isFinalTrivial())
            { cout << indent << "return;" << endl; }
            else
            {
                for(int i = 0; i < dcount; ++i)
                {
                    if(!(*prop)[i])
                    {
                        int var = i / domsize;
                        int val = i - var * domsize;
                        cout << indent << "var_array[" << var << "].removeFromDomain(" << val << ");\n";
                    }
                }
            }
        }
        else
        {
            int var = branchval / domsize;
            int val = branchval - var * domsize;

            if(true_b->isFinalTrivial())
            {
                cout << indent << "if(!var_array[" << var << "].inDomain(" << val << ")) {" << endl;
                deleted_vals.push_back(branchval);
                false_b->print(indent + "  ", deleted_vals);
                cout << indent << "}" << endl;
            }
            else
            {
                cout << indent << "if(var_array[" << var << "].inDomain(" << val << ")) {" << endl;
                true_b->print(indent + "  ", deleted_vals);

                if(!false_b->isFinalTrivial())
                {
                    cout << indent << "} else {" << endl;
                    deleted_vals.push_back(branchval);
                    false_b->print(indent + "  ", deleted_vals);
                }
                cout << indent << "}" << endl;
            }
        }
    }
};

typedef pair<array<size_t, dcount>, array<size_t, dcount> > counts;

counts data_count(const vector<DataPoint*>& data)
{
    array<size_t, dcount> false_counts;
    array<size_t, dcount> true_counts;
    for(int i = 0; i < dcount; ++i)
    {
        false_counts[i] = 0;
        true_counts[i] = 0;
    }

    for(size_t i = 0; i < data.size(); ++i)
    {
        OutData& ar = data[i]->out;
        for(size_t j = 0; j < ar.size(); ++j)
        {
            switch(ar[j])
            {
                case TT:
                    true_counts[j]++;
                    break;
                case TF:
                    false_counts[j]++;
                    break;
                default:
                    break;
            }
        }
   }
   return make_pair(false_counts, true_counts);
}

inline double e_log(double d)
{
    if(d == 0)
        return 0;
    else return d * log2(d);
}

template<typename T>
double get_entropy(const T& data, int pos)
{
    DataSplit s = split_data(data, pos);

    
    counts c_true = data_count(s.true_data.maps);
    counts c_false = data_count(s.false_data.maps);

    double entropy = 0;

    for(int i = 0; i < c_true.first.size(); ++i)
    {
        entropy += e_log(c_true.first[i]/(1.0*data.maps.size()));
        entropy += e_log(c_true.second[i]/(1.0*data.maps.size()));
        entropy += e_log(c_false.first[i]/(1.0*data.maps.size()));
        entropy += e_log(c_false.second[i]/(1.0*data.maps.size()));
    }

    return entropy;
}


pair<bool, InData> can_finish_tree(const counts& c)
{
    InData od;
    clear(od);
    for(size_t i = 0; i < c.first.size(); ++i)
    {
        if(c.first[i] != 0 && c.second[i] != 0)
            return make_pair(false, od);

        if(c.first[i] == 0)
            od[i] = true;
        else
            od[i] = false;
    }
    return make_pair(true, od);
}

bool is_fail_node(const counts& c)
{
    for(size_t i = 0; i < c.first.size(); ++i)
    {
        if(c.second[i] != 0)
            return false;
    }
    return true;
}




TreeVertex* build_tree(const DataList& data)
{
    counts c = data_count(data.maps);
    pair<bool, InData> out = can_finish_tree(c);
    if(out.first)
    {
        // Check if we can fail at this node.
        if(is_fail_node(c))
            return TreeVertex::FailTreeVertex();
        else
            return new TreeVertex(out.second);
    }
    else
    {
        TreeVertex* min_vertex = NULL;
        // Split on pos;
        for(int pos = 0; pos < dcount; ++pos)
        {
            DataSplit s = split_data(data, pos);
            if(!s.true_data.empty() && !s.false_data.empty())
            {
                TreeVertex* true_v = build_tree(s.true_data);
                TreeVertex* false_v = build_tree(s.false_data);
                TreeVertex* ret = new TreeVertex(true_v, false_v, pos);
#ifdef MIN_DEPTH
                if(min_vertex == NULL || ret->depth() < min_vertex->depth()
                   || (ret->depth() == min_vertex->depth() && ret->size() < min_vertex->size()))
#else
                if(min_vertex == NULL || ret->size() < min_vertex->size()
                   || (ret->size() == min_vertex->size() && ret->depth() < min_vertex->depth()))
#endif
                {
                    delete min_vertex;
                    min_vertex = ret;
                }
                else
                    delete ret;
            }
        }
        return min_vertex;
    }
}

TreeVertex* build_tree_heuristic(const DataList& data)
{
    counts c = data_count(data.maps);
    pair<bool, InData> out = can_finish_tree(c);
    if(out.first)
    {
        // Check if we can fail at this node.
        if(is_fail_node(c))
            return TreeVertex::FailTreeVertex();
        else
            return new TreeVertex(out.second);
    }
    else
    {
        double entropy = get_entropy(data, 0);
        int entropy_pos = 0;
        for(int pos = 1; pos < dcount; ++pos)
        {
            double new_entropy = get_entropy(data, pos);
            if(new_entropy < entropy)
            {
                entropy = new_entropy;
                entropy_pos = pos;
            }
        }

        DataSplit s = split_data(data, entropy_pos);
        assert(!s.true_data.empty() && !s.false_data.empty());
        TreeVertex* true_v = build_tree_heuristic(s.true_data);
        TreeVertex* false_v = build_tree_heuristic(s.false_data);
        TreeVertex* min_vertex = new TreeVertex(true_v, false_v, entropy_pos);
        return min_vertex;
    }

}

void output_tree(TreeVertex* v, const DataList& data, string indent = "")
{
    counts c = data_count(data.maps);
    pair<bool, InData> out = can_finish_tree(c);
    bool fail_node = is_fail_node(c);

    cout << indent << c << ":" << out.first << fail_node << ":" << out.second << ":" << endl;
    if(!v->isFinal())
    {
        DataSplit s = split_data(data, v->branchval);
        output_tree(v->true_b, s.true_data, indent + " ");
        cout << indent << "--" << endl;
        output_tree(v->false_b, s.false_data, indent + " ");

    }

}



int main(int argc, char** argv)
{
    DataList data = build_datapoints(CurrentConstraint());

    cerr << "Finished building datapoints" << endl;

    if(argc > 1 && argv[1] == string("-dumpcon"))
    {
        vector<array<int, vcount>> tuples = build_constraint(CurrentConstraint());

    }

    if(argc > 1 && argv[1] == string("-split"))
    {
        cout << "switch(lit) {\n";
        for(int i = 0; i < dcount; ++i)
        {
            cout << "case " << i << ":\n";
            DataList lit_data = filterDataList(data, i, false);
            TreeVertex* v= build_tree(lit_data);
            v->print();
           // output_tree(v, lit_data);
           cout << "break;\n";
        }
        cout << "default: abort(); }\n";
    }
    else if (argc > 1 && argv[1] == string("-heuristic"))
    {
        TreeVertex* v = build_tree_heuristic(data);
        cerr << "Tree depth:" << v->depth() << "  Tree size: " << v->size() << endl;
        v->print();
       // output_tree(v, data);
    }
    else
    {
        TreeVertex* v = build_tree(data);
        cerr << "Tree depth:" << v->depth() << "  Tree size: " << v->size() << endl;
        v->print();
       // output_tree(v, data);


    }
}


