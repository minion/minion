#ifndef TABLE_COMMON_H
#define TABLE_COMMON_H

struct Literal {
  SysInt var;
  DomainInt val;
  Literal(SysInt _var, DomainInt _val) : var(_var), val(_val) {}
};

class BaseTableData {
protected:
  TupleList* tuple_data;

public:
  DomainInt getVarCount() {
    return tuple_data->tuple_size();
  }

  DomainInt getNumOfTuples() {
    return tuple_data->size();
  }

  DomainInt getLiteralPos(Literal l) {
    return tuple_data->get_literal(l.var, l.val);
  }

  DomainInt* getPointer() {
    return tuple_data->getPointer();
  }

  DomainInt getLiteralCount() {
    return tuple_data->literal_num;
  }

  Literal getLiteralFromPos(SysInt pos) {
    pair<SysInt, DomainInt> lit = tuple_data->get_varval_from_literal(pos);
    return Literal(lit.first, lit.second);
  }

  pair<DomainInt, DomainInt> getDomainBounds(SysInt var) {
    return make_pair(tuple_data->dom_smallest[var],
                     tuple_data->dom_smallest[var] + tuple_data->dom_size[var] - 1); // CHECK -1
  }

  BaseTableData(TupleList* _tuple_data) : tuple_data(_tuple_data) {}
};

class TrieData : public BaseTableData {

public:
  TupleTrieArray* tupleTrieArrayptr;

  TrieData(TupleList* _tuple_data)
      : BaseTableData(_tuple_data), tupleTrieArrayptr(_tuple_data->getTries()) {}

  // TODO: Optimise possibly?
  bool checkTuple(DomainInt* tuple, SysInt tuple_size) {
    D_ASSERT(tuple_size == getVarCount());
    for(SysInt i = 0; i < getNumOfTuples(); ++i) {
      if(std::equal(tuple, tuple + tuple_size, tuple_data->get_tupleptr(i)))
        return true;
    }
    return false;
  }
};


inline TupleTrieArray* TupleList::getTries() {
  if(triearray == NULL)
    triearray = new TupleTrieArray(this);
  return triearray;
}

#endif