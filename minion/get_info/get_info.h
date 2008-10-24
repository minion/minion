
#ifndef GET_INFO_H_FLF
#define GET_INFO_H_FLF

 enum Info_VarEvent {
#define VAR_EVENT(x,y) VAR_INFO_##x ,
#include "PropEvents.h"
#undef VAR_EVENT
  VarEvent_END
  };


 enum Info_VarType
 { VAR_INFO_BOOL , VAR_INFO_BOUNDVAR, VAR_INFO_SPARSEBOUND, VAR_INFO_RANGEVAR ,
 VAR_INFO_BIGRANGEVAR, VarType_END
 };

enum Info_ConEvent {
CON_INFO_StaticTrigger,
CON_INFO_DynamicTrigger,
CON_INFO_SpecialTrigger,
CON_INFO_DynamicMovePtr,
CON_INFO_AddSpecialToQueue,
CON_INFO_AddConToQueue,
CON_INFO_AddDynToQueue,
CON_INFO_SearchTrie,
CON_INFO_LoopSearchTrie,
ConEvent_END
};
 
enum Info_PropEvent {
#define PROP_EVENT(x) PROP_INFO_##x ,
#include "PropEvents.h"
#undef PROP_EVENT
PropEvent_END
 };
 
const int VarEventCount=VarEvent_END;
const int VarTypeCount=VarType_END;
const int ConEventCount=ConEvent_END;
const int PropEventCount=PropEvent_END;

#ifdef MORE_SEARCH_INFO

#define VAR_INFO_ADDONE(VarType, VarEvent) \
VarInfoAddone(VarType, VAR_INFO_ ## VarEvent)

#define CON_INFO_ADDONE(ConEvent) \
ConInfoAddone(CON_INFO_ ## ConEvent)

#define PROP_INFO_ADDONE(PropEvent) \
PropInfoAddone(PROP_INFO_ ## PropEvent)

void VarInfoAddone(Info_VarType, Info_VarEvent);
void ConInfoAddone(Info_ConEvent);
void PropInfoAddone(Info_PropEvent);
void print_search_info();

#else
#define CON_INFO_ADDONE(ConEvent)
#define VAR_INFO_ADDONE(VarType, VarEvent)
#define PROP_INFO_ADDONE(PropType)
#endif

#endif
