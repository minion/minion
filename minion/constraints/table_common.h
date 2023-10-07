#ifndef TABLE_COMMON_H
#define TABLE_COMMON_H

struct Literal {
  SysInt var;
  DomainInt val;
  Literal(SysInt _var, DomainInt _val) : var(_var), val(_val) {}
};

class BaseTableData {
protected:
  std::shared_ptr<TupleList> tupleData;

public:
  DomainInt getVarCount() {
    return tupleData->tupleSize();
  }

  DomainInt getNumOfTuples() {
    return tupleData->size();
  }

  DomainInt getLiteralPos(Literal l) {
    return tupleData->getLiteral(l.var, l.val);
  }

  DomainInt* getPointer() {
    return tupleData->getPointer();
  }

  DomainInt getLiteralCount() {
    return tupleData->literalNum;
  }

  Literal getLiteralFromPos(SysInt pos) {
    pair<SysInt, DomainInt> lit = tupleData->getVarvalFromLiteral(pos);
    return Literal(lit.first, lit.second);
  }

  pair<DomainInt, DomainInt> getDomainBounds(SysInt var) {
    return make_pair(tupleData->domSmallest[var],
                     tupleData->domSmallest[var] + tupleData->domSize[var] - 1); // CHECK -1
  }

  BaseTableData(std::shared_ptr<TupleList> _tupleData) : tupleData(_tupleData) {}
};

class TrieData : public BaseTableData {

public:
  std::shared_ptr<TupleTrieArray> tupleTrieArrayptr;

  TrieData(std::shared_ptr<TupleList> _tupleData)
      : BaseTableData(_tupleData), tupleTrieArrayptr(getTries(_tupleData)) {}

  // TODO: Optimise possibly?
  bool checkTuple(DomainInt* tuple, SysInt tupleSize) {
    D_ASSERT(tupleSize == getVarCount());
    for(SysInt i = 0; i < getNumOfTuples(); ++i) {
      if(std::equal(tuple, tuple + tupleSize, tupleData->getTupleptr(i)))
        return true;
    }
    return false;
  }
};

std::shared_ptr<TupleTrieArray> getTries(std::shared_ptr<TupleList> tl) {
  if(tl->triearray == NULL)
    tl->triearray = std::make_shared<TupleTrieArray>(tl);
  return tl->triearray;
}

#endif