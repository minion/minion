template<typename WrapType, VarType VAR_TYPE>
struct InfoRefType
{
  WrapType data;
  
  static const BOOL isBool = WrapType::isBool;
  static const BoundType isBoundConst = WrapType::isBoundConst;

  BOOL isBound()
  { return data.isBound();}
  
  InfoRefType(const WrapType& _data) : data(_data)
  { VarInfoAddone(VAR_TYPE, "copy"); }
  
  InfoRefType() 
  {VarInfoAddone(VAR_TYPE, "construct");}
  
  InfoRefType(const InfoRefType& b) : data(b.data)
  {VarInfoAddone(VAR_TYPE, "copy");}
  
  BOOL isAssigned()
  { 
    VarInfoAddone(VAR_TYPE, "isAssigned");
    return data.isAssigned(); 
  }
  
  DomainInt getAssignedValue()
  { 
    VarInfoAddone(VAR_TYPE, "getAssignedValue");
    return data.getAssignedValue(); }
  
  BOOL isAssignedValue(DomainInt i)
  { 
    VarInfoAddone(VAR_TYPE, "isAssignedValue");
    return data.isAssignedValue(i);
  }
  
  BOOL inDomain(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, "inDomain");
    return data.inDomain( b); 
  }
  
  BOOL inDomain_noBoundCheck(DomainInt b)
  {
    VarInfoAddone(VAR_TYPE, "inDomain_noBoundCheck");
    return data.inDomain_noBoundCheck(b);
  }
  
  
  DomainInt getMax()
  {
    VarInfoAddone(VAR_TYPE, "getMax");
    return data.getMax(); 
  }
  
  DomainInt getMin()
  { 
    VarInfoAddone(VAR_TYPE, "getMin");
    return data.getMin(); 
  }

  DomainInt getInitialMax() const
  {
    VarInfoAddone(VAR_TYPE, "getInitialMax");
    return data.getInitialMax(); 
  }
  
  DomainInt getInitialMin() const
  { 
    VarInfoAddone(VAR_TYPE, "getInitialMin");
    return data.getInitialMin(); 
  }
  
  void setMax(DomainInt i)
  { 
    VarInfoAddone(VAR_TYPE, "setMax");
    data.setMax(i); 
  }
  
  void setMin(DomainInt i)
  { 
    VarInfoAddone(VAR_TYPE, "setMin");
    data.setMin(i); 
  }
  
  void uncheckedAssign(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, "uncheckedAssign");
    data.uncheckedAssign( b); 
  }
  
  void propagateAssign(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, "propagateAssign");
    data.propagateAssign( b); 
  }
  
  void removeFromDomain(DomainInt b)
  { 
    VarInfoAddone(VAR_TYPE, "removeFromDomain");
    data.removeFromDomain( b); 
  }
  
  void addTrigger(Trigger t, TrigType type)
  {
    VarInfoAddone(VAR_TYPE, "addTrigger");
    data.addTrigger( t, type); 
  }
  
  friend std::ostream& operator<<(std::ostream& o, const InfoRefType& ir)
  {
    return o << "InfoRef " << ir.data;
  }
 
  int getDomainChange(DomainDelta d)
  { 
    VarInfoAddone(VAR_TYPE, "getDomainChange");
    return d.XXX_get_domain_diff(); 
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, DomainInt pos = -999)
  { 
    VarInfoAddone(VAR_TYPE, "addDynamicTrigger");
    data.addDynamicTrigger( t, type, pos); 
  }
#endif
};


