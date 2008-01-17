

// Can't instantiate a template with a string so must have this VarType
// to represent the string with an integer.

enum VarType
 { VAR_INFO_BOOL , VAR_INFO_BOUNDVAR, VAR_INFO_SPARSEBOUND, VAR_INFO_RANGEVAR ,
 VAR_INFO_BIGRANGEVAR
 };


// These four symbols are either defined as procedures here or as
// macros in minion.h
void VarInfoAddone(VarType, string);
void ConInfoAddone(string);
void PropInfoAddone(string);
void print_search_info();
