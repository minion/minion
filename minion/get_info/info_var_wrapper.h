template<typename WrapType, VarType VAR_TYPE>
struct InfoRefType
{
  WrapType data;
  
  static const BOOL isBool = WrapType::isBool;
  static const BoundType isBoundConst = WrapType::isBoundConst;

  BOOL isBound()
  { return data.isBound();}
  
  InfoRefType(const WrapType& _data) : data(_data)
  { VAR_INFO_ADDONE(VAR_TYPE, copy); }
  
  InfoRefType() 
  {VAR_INFO_ADDONE(VAR_TYPE, construct);}
  
  InfoRefType(const InfoRefType& b) : data(b.data)
  {VAR_INFO_ADDONE(VAR_TYPE, copy);}
  
  BOOL isAssigned()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssigned);
    return data.isAssigned(); 
  }
  
  int getAssignedValue()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getAssignedValue);
    return data.getAssignedValue(); }
  
  BOOL isAssignedValue(int i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, isAssignedValue);
    return data.isAssignedValue(i);
  }
  
  BOOL inDomain(int b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, inDomain);
    return data.inDomain( b); 
  }
  
  BOOL inDomain_noBoundCheck(int b)
  {
    VAR_INFO_ADDONE(VAR_TYPE, inDomain_noBoundCheck);
    return data.inDomain_noBoundCheck(b);
  }
  
  
  int getMax()
  {
    VAR_INFO_ADDONE(VAR_TYPE, getMax);
    return data.getMax(); 
  }
  
  int getMin()
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getMin);
    return data.getMin(); 
  }

  int getInitialMax() const
  {
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMax);
    return data.getInitialMax(); 
  }
  
  int getInitialMin() const
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getInitialMin);
    return data.getInitialMin(); 
  }
  
  void setMax(int i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMax);
    data.setMax(i); 
  }
  
  void setMin(int i)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, setMin);
    data.setMin(i); 
  }
  
  void uncheckedAssign(int b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, uncheckedAssign);
    data.uncheckedAssign( b); 
  }
  
  void propogateAssign(int b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, propogateAssign);
    data.propogateAssign( b); 
  }
  
  void removeFromDomain(int b)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, RemoveFromDomain);
    data.removeFromDomain( b); 
  }
  
  void addTrigger(Trigger t, TrigType type)
  {
    VAR_INFO_ADDONE(VAR_TYPE, addTrigger);
    data.addTrigger( t, type); 
  }
  
  operator string()
  { return "InfoRef:" + string(data); }
  
  int getDomainChange(DomainDelta d)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, getDomainChange);
    return d.XXX_get_domain_diff(); 
  }
  
#ifdef DYNAMICTRIGGERS
  void addDynamicTrigger(DynamicTrigger* t, TrigType type, int pos = -999)
  { 
    VAR_INFO_ADDONE(VAR_TYPE, addDynamicTrigger);
    data.addDynamicTrigger( t, type, pos); 
  }
#endif
};


