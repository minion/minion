// To add a new VarInfoAddone call here, you need to add a new event to the
// enum in get_info.h and also put the name of the new event in the appropriate
// place in a vector in get_info.cpp.

template<typename WrapType, VarType VAR_TYPE>
struct InfoRefType
{
  WrapType data;
  
  static const BOOL isBool = WrapType::isBool;
  static const BoundType isBoundConst = WrapType::isBoundConst;

  BOOL isBound()
  { return data.isBound();}
  
  InfoRefType(const WrapType& _data) : data(_data)
  { VarInfoAddone(VAR_TYPE, VAR_INFO_copy); }
  
  InfoRefType() 
  {VarInfoAddone(VAR_TYPE, VAR_INFO_construct);}
  
  InfoRefType(const InfoRefType& b) : data(b.data)
  {VarInfoAddone(VAR_TYPE, VAR_INFO_copy);}
  
  BOOL isAssigned()
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_isAssigned);
    return data.isAssigned(); 
  }
  
  DomainInt getAssignedValue()
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_getAssignedValue);
    return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_isAssignedValue);
    return data.isAssignedValue(i);
  }
  
  BOOL inDomain(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_inDomain);
    return data.inDomain( b); 
  }
  
  BOOL inDomain_noBoundCheck(DomainInt b)
  {
    VarInfoAddone(VAR_TYPE, VAR_INFO_inDomain_noBoundCheck);
    return data.inDomain_noBoundCheck(b);
  }
  
  
  DomainInt getMax()
  {
    VarInfoAddone(VAR_TYPE, VAR_INFO_getMax);
    return data.getMax(); 
  }
  
  DomainInt getMin()
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_getMin);
    return data.getMin(); 
  }

  DomainInt getInitialMax() const
  {
    VarInfoAddone(VAR_TYPE, VAR_INFO_getInitialMax);
    return data.getInitialMax(); 
  }
  
  DomainInt getInitialMin() const
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_getInitialMin);
    return data.getInitialMin(); 
  }
  
  void setMax(DomainInt i)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_setMax);
    data.setMax(i); 
  }
  
  void setMin(DomainInt i)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_setMin);
    data.setMin(i); 
  }
  
  void uncheckedAssign(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_uncheckedAssign);
    data.uncheckedAssign( b); 
  }
  
  void propagateAssign(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_propagateAssign);
    data.propagateAssign( b); 
  }
  
  void removeFromDomain(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_removeFromDomain);
    data.removeFromDomain( b); 
  }
  
  void addTrigger(Trigger t, TrigType type)
  {
    VarInfoAddone(VAR_TYPE, VAR_INFO_addTrigger);
    data.addTrigger( t, type); 
  }
  
  friend std::ostream& operator<<(std::ostream& o, const InfoRefType& ir)
  {
    return o << "InfoRef " << ir.data;
  }
 
  int getDomainChange(DomainDelta d)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_getDomainChange);
    return d.XXX_get_domain_diff(); 
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  { 
    VarInfoAddone(VAR_TYPE, VAR_INFO_addDynamicTrigger);
    data.addDynamicTrigger( t, type, pos); 
  }
#endif
};


