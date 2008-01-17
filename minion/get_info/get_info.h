
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
  VAR_INFO_propogateAssign,
  VAR_INFO_RemoveFromDomain,
  VAR_INFO_addTrigger,
  VAR_INFO_getDomainChange,
  VAR_INFO_addDynamicTrigger,
  VarEvent_END
  };


 enum VarType
 { VAR_INFO_BOOL , VAR_INFO_BOUNDVAR, VAR_INFO_SPARSEBOUND, VAR_INFO_RANGEVAR ,
 VAR_INFO_BIGRANGEVAR, VarType_END
 };

enum ConEvent {
CON_INFO_StaticTrigger,
CON_INFO_DynamicTrigger,
CON_INFO_SpecialTrigger,
CON_INFO_DynamicMovePtr,
CON_INFO_AddSpecialToQueue,
CON_INFO_AddConToQueue,
CON_INFO_AddDynToQueue,
ConEvent_END
};
 
 enum PropEvent {
 PROP_INFO_CheckAssign,
 PROP_INFO_BoundTable,
 PROP_INFO_Reify,
 PROP_INFO_ReifyTrue,
 PROP_INFO_Table,
 PROP_INFO_ArrayNeq,
 PROP_INFO_BinaryNeq,
 PROP_INFO_NonGACElement,
 PROP_INFO_GACElement,
 PROP_INFO_Lex,
 PROP_INFO_FullSum,
 PROP_INFO_BoolSum,
 PROP_INFO_LightSum,
 PROP_INFO_WeightBoolSum,
 PROP_INFO_ReifyEqual,
 PROP_INFO_Equal,
 PROP_INFO_BinaryLeq,
 PROP_INFO_Min,
 PROP_INFO_OccEqual,
 PROP_INFO_Pow,
 PROP_INFO_And,
 PROP_INFO_Product,
 PROP_INFO_DynSum,
 PROP_INFO_DynSumSat,
 PROP_INFO_Dyn3SAT,
 PROP_INFO_Dyn2SAT,
 PROP_INFO_DynLitWatch,
 PROP_INFO_DynElement,
 PROP_INFO_DynVecNeq,
 PROP_INFO_DynGACTable,
 PROP_INFO_Mod,
 PropEvent_END
 


 };
 
const int VarEventCount=VarEvent_END;
const int VarTypeCount=VarType_END;
const int ConEventCount=ConEvent_END;
const int PropEventCount=PropEvent_END;

#define VAR_INFO_ADDONE(VarType, VarEvent) \
VarInfoAddone(VarType, VAR_INFO_ ## VarEvent)

#define CON_INFO_ADDONE(ConEvent) \
ConInfoAddone(CON_INFO_ ## ConEvent)

#define PROP_INFO_ADDONE(PropEvent) \
PropInfoAddone(PROP_INFO_ ## PropEvent)

void VarInfoAddone(VarType, VarEvent);
void ConInfoAddone(ConEvent);
void PropInfoAddone(PropEvent);
void print_search_info();
