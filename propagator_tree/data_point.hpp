#include <array>
using namespace std;

enum TriBool
{ TT, TF, TM };

typedef array<bool, dcount> InData;
typedef array<TriBool, dcount> OutData;

struct DataPoint
{
    InData in;
    OutData out;

    bool isWipeout() const
    {
        for(int i = 0; i < dcount; ++i)
        {
            if(out[i] == TT)
                return false;
        }
        return true;
    }

    DataPoint(const InData& _in, const OutData& _out)
        : in(_in), out(_out)
    { }

    DataPoint() = default;
    DataPoint(const DataPoint&) = default;
};

template<typename T, typename U>
std::ostream& operator<<(std::ostream& out, const std::pair<T, U>& p)
{ return out << "(" << p.first << "," << p.second << ")"; }

template<typename T, size_t s>
std::ostream& operator<<(std::ostream& out, const std::array<T, s>& a)
{
    out << "[";
    for(size_t i = 0; i < s; ++i)
        out << a[i] << ",";
    out << "]";
    return out;
}

ostream& operator<<(ostream& os, InData& id)
{
    os << "[";
    for(size_t i = 0; i < id.size(); ++i)
        os << id[i];
    os << "]";
    return os;
}

ostream& operator<<(ostream& os, OutData& od)
{
    os << "[";
    for(size_t i = 0; i < od.size(); ++i)
    {
        switch(od[i])
        {
            case TT:
                os << "T";
                break;
            case TF:
                os << "F";
                break;
            case TM:
                os << ".";
                break;
        }
    }
    os << "]";
    return os;
}

struct DataList
{
    vector<DataPoint*> maps;
    vector<InData*> failnodes;

    void insertDP(const DataPoint& dp)
    {
        if(dp.isWipeout())
        {
            failnodes.push_back(new InData(dp.in));
        }
        else
        {
            maps.push_back(new DataPoint(dp));
        }
    }

    bool empty() const
    { return maps.empty() && failnodes.empty(); }

    DataList(const vector<DataPoint*>& _m,
             const vector<InData*>& _f)
        : maps(_m), failnodes(_f)
    { }

    DataList()
    {
        maps.reserve(50);
        failnodes.reserve(50);
    }
    DataList(const DataList&) = default;

    void clear()
    {
        maps.clear();
        failnodes.clear();
    }
};

DataList filterDataList(const DataList& data, int pos, bool val)
{
    DataList ret_data;
     for(size_t i = 0; i < data.maps.size(); ++i)
    {
        if(data.maps[i]->in[pos] == val)
            ret_data.maps.push_back(data.maps[i]);
    }

    for(size_t i = 0; i < data.failnodes.size(); ++i)
    {
        if((*data.failnodes[i])[pos] == val)
            ret_data.failnodes.push_back(data.failnodes[i]);
    }

    return ret_data;
}

struct DataSplit
{
    DataList false_data;
    DataList true_data;

    void clear()
    {
        false_data.clear();
        true_data.clear();
    }
};

// We pass this in just to avoid excessive malloc/deallocing
void split_data(const DataList& data, int pos, DataSplit& ret_data)
{
    ret_data.clear();
    for(size_t i = 0; i < data.maps.size(); ++i)
    {
        if(data.maps[i]->in[pos])
            ret_data.true_data.maps.push_back(data.maps[i]);
        else
            ret_data.false_data.maps.push_back(data.maps[i]);
    }

    for(size_t i = 0; i < data.failnodes.size(); ++i)
    {
        if((*data.failnodes[i])[pos])
            ret_data.true_data.failnodes.push_back(data.failnodes[i]);
        else
            ret_data.false_data.failnodes.push_back(data.failnodes[i]);
    }
}

ostream& operator<<(ostream& os, const DataPoint& dp)
{ return os << "(" << dp.in << "," << dp.out << ")"; }

ostream& operator<<(ostream& os, const DataList& dl)
{
    for(size_t i = 0; i < dl.maps.size(); ++i)
        os << *(dl.maps[i]) << endl;

    for(size_t i = 0; i < dl.failnodes.size(); ++i)
        os << *(dl.failnodes[i]) << endl;

    return os;
}


