#ifndef DEFINED_MACROS
#define DEFINED_MACROS

// In an ideal world, this would be auto-generated, and involve less typing. Unfortunatly
// it must be manually updated instead.

inline void print_macros(void) {
#ifdef BINARY_SEARCH
cout << "BINARY_SEARCH" << " "; 
#endif

#ifdef BOUNDS_CHECK
cout << "BOUNDS_CHECK" << " "; 
#endif

#ifdef DEBUG
cout << "DEBUG" << " "; 
#endif
#ifdef DYNAMICTRIGGERS
cout << "DYNAMICTRIGGERS" << " "; 
#endif
#ifdef FULL_DOMAIN_TRIGGERS
cout << "FULL_DOMAIN_TRIGGERS" << " "; 
#endif
#ifdef LIGHT_VECTOR
cout << "LIGHT_VECTOR" << " "; 
#endif
#ifdef LISTPERLIT
cout << "LISTPERLIT" << " "; 
#endif
#ifdef MORE_SEARCH_INFO
cout << "MORE_SEARCH_INFO" << " "; 
#endif
#ifdef NIGHTINGALE
cout << "NIGHTINGALE" << " "; 
#endif
#ifdef NOCATCH
cout << "NOCATCH" << " "; 
#endif
#ifdef NO_DEBUG
cout << "NO_DEBUG" << " "; 
#endif
#ifdef NO_DYN_CHECK
cout << "NO_DYN_CHECK" << " "; 
#endif
#ifdef NO_PRINT
cout << "NO_PRINT" << " "; 
#endif
#ifdef OLDTABLE
cout << "OLDTABLE" << " "; 
#endif
#ifdef PRIV
cout << "PRIV" << " "; 
#endif
#ifdef QUICK_COMPILE
cout << "QUICK_COMPILE" << " "; 
#endif
#ifdef REENTER
cout << "REENTER" << " "; 
#endif
#ifdef MANY_VAR_CONTAINERS
cout << "MANY_VAR_CONTAINERS" << " "; 
#endif
#ifdef REGINLHOMME
cout << "REGINLHOMME" << " "; 
#endif
#ifdef SATSPECIAL1
cout << "SATSPECIAL1" << " "; 
#endif
#ifdef SATSPECIAL2
cout << "SATSPECIAL2" << " "; 
#endif
#ifdef SATSPECIAL3
cout << "SATSPECIAL3" << " "; 
#endif
#ifdef SLOW_TRIGGERS
cout << "SLOW_TRIGGERS" << " "; 
#endif
#ifdef USELIGHTVECTOR
cout << "USELIGHTVECTOR" << " "; 
#endif
#ifdef USE_HASHTABLE
cout << "USE_HASHTABLE" << " "; 
#endif
#ifdef USE_HASHSET
cout << "USE_HASHSET" << " "; 
#endif
#ifdef USE_SETJMP
cout << "USE_SETJMP" << " "; 
#endif
#ifdef WATCHEDLITERALS
cout << "WATCHEDLITERALS" << " "; 
#endif
#ifdef _MSC_VER
cout << "_MSC_VER" << " "; 
#endif
#ifdef __GNUC__
cout << "__GNUC__" << " "; 
#endif
cout << endl;
}

inline void print_constraints() {
#ifdef CT_ELEMENT_ABC
      cout << "element ";
#endif
#ifdef CT_WATCHED_ELEMENT_ABC
      cout << "watchelement ";
#endif
#ifdef CT_GACELEMENT_ABC
      cout << "gacelement-deprecated ";
#endif
#ifdef CT_ELEMENT_ONE_ABC
      cout << "element_one ";
#endif
#ifdef CT_WATCHED_ELEMENT_ONE_ABC
      cout << "watchelement_one ";
#endif
#ifdef CT_ALLDIFF_ABC
      cout << "alldiff ";
#endif
#ifdef CT_GACALLDIFF_ABC
      cout << "gacalldiff ";
#endif
#ifdef CT_WATCHED_NEQ_ABC
      cout << "watchneq ";
#endif
#ifdef CT_DISEQ_ABC
      cout << "diseq ";
#endif
#ifdef CT_DISEQ_REIFY_ABC
      cout << "__reify_diseq ";
#endif
#ifdef CT_EQ_ABC
      cout << "eq ";
#endif
#ifdef CT_EQ_REIFY_ABC
      cout << "__reify_eq ";
#endif
#ifdef CT_MINUSEQ_ABC
      cout << "minuseq ";
#endif
#ifdef CT_MINUSEQ_REIFY_ABC
      cout << "__reify_minuseq ";
#endif
#ifdef CT_ABS_ABC
      cout << "abs ";
#endif
#ifdef CT_INEQ_ABC
      cout << "ineq ";
#endif
#ifdef CT_WATCHED_LESS_ABC
      cout << "watchless ";
#endif
#ifdef CT_LEXLEQ_ABC
      cout << "lexleq ";
#endif
#ifdef CT_LEXLESS_ABC
      cout << "lexless ";
#endif
#ifdef CT_MAX_ABC
      cout << "max ";
#endif
#ifdef CT_MIN_ABC
      cout << "min ";
#endif
#ifdef CT_OCCURRENCE_ABC
      cout << "occurrence ";
#endif
#ifdef CT_LEQ_OCCURRENCE_ABC
      cout << "occurrenceleq ";
#endif
#ifdef CT_GEQ_OCCURRENCE_ABC
      cout << "occurrencegeq ";
#endif
#ifdef CT_PRODUCT2_ABC
      cout << "product ";
#endif
#ifdef CT_DIFFERENCE_ABC
      cout << "difference ";
#endif
#ifdef CT_WEIGHTGEQSUM_ABC
      cout << "weightedsumgeq ";
#endif
#ifdef CT_WEIGHTLEQSUM_ABC
      cout << "weightedsumleq ";
#endif
#ifdef CT_GEQSUM_ABC
      cout << "sumgeq ";
#endif
#ifdef CT_WATCHED_GEQSUM_ABC
      cout << "watchsumgeq ";
#endif
#ifdef CT_LEQSUM_ABC
      cout << "sumleq ";
#endif
#ifdef CT_WATCHED_LEQSUM_ABC
      cout << "watchsumleq ";
#endif
#ifdef CT_WATCHED_TABLE_ABC
      cout << "table ";
#endif
#ifdef CT_WATCHED_NEGATIVE_TABLE_ABC
      cout << "negativetable ";
#endif
#ifdef CT_WATCHED_VECNEQ_ABC
      cout << "watchvecneq ";
#endif
#ifdef CT_WATCHED_LITSUM_ABC
      cout << "litsumgeq ";
#endif
#ifdef CT_POW_ABC
      cout << "pow ";
#endif
#ifdef CT_DIV_ABC
      cout << "div ";
#endif
#ifdef CT_MODULO_ABC
      cout << "modulo ";
#endif
#ifdef CT_WATCHED_VEC_OR_AND_ABC
      cout << "watchvecexists_and ";
#endif
#ifdef CT_WATCHED_VEC_OR_LESS_ABC
      cout << "watchvecexists_less ";
#endif
#ifdef CT_WATCHED_HAMMING_ABC
      cout << "hamming ";
#endif
#ifdef CT_WATCHED_OR_ABC
      cout << "disabled-or ";
#endif
#ifdef CT_WATCHED_NEW_OR_ABC
      cout << "watched-or ";
#endif
#ifdef CT_WATCHED_NEW_AND_ABC
      cout << "watched-and ";
#endif
#ifdef CT_GADGET_ABC
      cout << "gadget ";
#endif
#ifdef CT_WATCHED_INSET_ABC
      cout << "w-inset ";
#endif
#ifdef CT_WATCHED_NOT_INSET_ABC
      cout << "w-notinset ";
#endif
#ifdef CT_WATCHED_INRANGE_ABC
      cout << "w-inrange ";
#endif
#ifdef CT_WATCHED_NOT_INRANGE_ABC
      cout << "w-notinrange ";
#endif
#ifdef CT_WATCHED_LIT_ABC
      cout << "w-literal ";
#endif
#ifdef CT_WATCHED_NOTLIT_ABC
      cout << "w-notliteral ";
#endif
#ifdef CT_REIFY_ABC
      cout << "reify ";
#endif
#ifdef CT_REIFYIMPLY_ABC
      cout << "reifyimply ";
#endif
#ifdef CT_REIFYIMPLY_QUICK_ABC
      cout << "reifyimply-quick ";
#endif
#ifdef CT_GCC_ABC
      cout << "gcc ";
#endif
    cout << endl;
}

#endif
