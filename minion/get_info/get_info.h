

// Can't instantiate a template with a string so must have this VarType
// to represent the string with an integer.

enum VarType
 { VAR_INFO_BOOL , VAR_INFO_BOUNDVAR, VAR_INFO_SPARSEBOUND, VAR_INFO_RANGEVAR ,
 VAR_INFO_BIGRANGEVAR, VarType_END
 };

  enum VarEvent {
 VAR_INFO_construct,
 VAR_INFO_copy,
 VAR_INFO_isAssigned,
 VAR_INFO_getAssignedValue,
   VAR_INFO_isAssignedValue,
   VAR_INFO_inDomain,
   VAR_INFO_inDomain_noBoundCheck,
   VAR_INFO_getMax,
   VAR_INFO_getMin,
   VAR_INFO_getInitialMax,
   VAR_INFO_getInitialMin,
  VAR_INFO_setMax,
  VAR_INFO_setMin,
  VAR_INFO_uncheckedAssign,
  VAR_INFO_propagateAssign,
  VAR_INFO_removeFromDomain,
  VAR_INFO_addTrigger,
  VAR_INFO_getDomainChange,
  VAR_INFO_addDynamicTrigger,
  VarEvent_END
  };

const int VarTypeSize=VarType_END;
const int VarEventSize=VarEvent_END;
// These four symbols are either defined as procedures here or as
// macros in minion.h
void VarInfoAddone(VarType, VarEvent);
void ConInfoAddone(string);
void PropInfoAddone(string);
void print_search_info();
