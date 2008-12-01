set(ALL_CONSTRAINTS "element" "element_one" "watchelement" "watchelement_one"
                    "gacelement-deprecated" "alldiff" "gacalldiff" "gcc" "watchneq"
                    "diseq" "__reify_diseq" "eq" "__reify_eq" "minuseq" "__reify_minuseq"
                    "abs" "ineq" "watchless" "lexleq" "lexless" "max" "min" "occurrence"
                    "occurrenceleq" "occurrencegeq" "product" "difference"
                    "weightedsumleq" "weightedsumgeq" "sumgeq" "sumleq" "watchsumgeq"
                    "watchsumleq" "table" "negativetable" "watchvecneq" "litsumgeq"
                    "pow" "div" "modulo" "gadget" "disabled-or" "watchvecexists_less"
                    "watchvecexists_and" "hamming" "watched-or" "watched-and"
                    "w-inset" "w-notinset" "w-inrange" "w-notinrange" "w-literal"
                    "w-notliteral" "reify" "reifyimply-quick" "reifyimply")

set(NAME_ID_element "CT_ELEMENT")
set(NAME_TYPE_element "STATIC_CT")
set(NAME_READ_element "read_list read_var read_var")

set(NAME_ID_element_one "CT_ELEMENT_ONE")
set(NAME_TYPE_element_one "STATIC_CT")
set(NAME_READ_element_one "read_list read_var read_var")

set(NAME_ID_watchelement "CT_WATCHED_ELEMENT")
set(NAME_TYPE_watchelement "DYNAMIC_CT")
set(NAME_READ_watchelement "read_list read_var read_var")

set(NAME_ID_watchelement_one "CT_WATCHED_ELEMENT_ONE")
set(NAME_TYPE_watchelement_one "DYNAMIC_CT")
set(NAME_READ_watchelement_one "read_list read_var read_var")

set(NAME_ID_gacelement-deprecated "CT_GACELEMENT")
set(NAME_TYPE_gacelement-deprecated "STATIC_CT")
set(NAME_READ_gacelement-deprecated "read_list read_var read_var")

set(NAME_ID_alldiff "CT_ALLDIFF")
set(NAME_TYPE_alldiff "STATIC_CT")
set(NAME_READ_alldiff "read_list")

set(NAME_ID_gacalldiff "CT_GACALLDIFF")
set(NAME_TYPE_gacalldiff "STATIC_CT")
set(NAME_READ_gacalldiff "read_list")

set(NAME_ID_gcc "CT_GCC")
set(NAME_TYPE_gcc "STATIC_CT")
set(NAME_READ_gcc "read_list read_constant_list read_list")

set(NAME_ID_watchneq "CT_WATCHED_NEQ")
set(NAME_TYPE_watchneq "DYNAMIC_CT")
set(NAME_READ_watchneq "read_var read_var")

set(NAME_ID_diseq "CT_DISEQ")
set(NAME_TYPE_diseq "STATIC_CT")
set(NAME_READ_diseq "read_var read_var")

set(NAME_ID___reify_diseq "CT_DISEQ_REIFY")
set(NAME_TYPE___reify_diseq "STATIC_CT")
set(NAME_READ___reify_diseq "read_var read_var read_bool_var")

set(NAME_ID_eq "CT_EQ")
set(NAME_TYPE_eq "STATIC_CT")
set(NAME_READ_eq "read_var read_var")

set(NAME_ID___reify_eq "CT_EQ_REIFY")
set(NAME_TYPE___reify_eq "STATIC_CT")
set(NAME_READ___reify_eq "read_var read_var read_bool_var")

set(NAME_ID_minuseq "CT_MINUSEQ")
set(NAME_TYPE_minuseq "STATIC_CT")
set(NAME_READ_minuseq "read_var read_var")

set(NAME_ID___reify_minuseq "CT_MINUSEQ_REIFY")
set(NAME_TYPE___reify_minuseq "STATIC_CT")
set(NAME_READ___reify_minuseq "read_var read_var read_bool_var")

set(NAME_ID_abs "CT_ABS")
set(NAME_TYPE_abs "STATIC_CT")
set(NAME_READ_abs "read_var read_var")

set(NAME_ID_ineq "CT_INEQ")
set(NAME_TYPE_ineq "STATIC_CT")
set(NAME_READ_ineq "read_var read_var read_constant")

set(NAME_ID_watchless "CT_WATCHED_LESS")
set(NAME_TYPE_watchless "DYNAMIC_CT")
set(NAME_READ_watchless "read_var read_var")

set(NAME_ID_lexleq "CT_LEXLEQ")
set(NAME_TYPE_lexleq "STATIC_CT")
set(NAME_READ_lexleq "read_list read_list")

set(NAME_ID_lexless "CT_LEXLESS")
set(NAME_TYPE_lexless "STATIC_CT")
set(NAME_READ_lexless "read_list read_list")

set(NAME_ID_max "CT_MAX")
set(NAME_TYPE_max "STATIC_CT")
set(NAME_READ_max "read_list read_var")

set(NAME_ID_min "CT_MIN")
set(NAME_TYPE_min "STATIC_CT")
set(NAME_READ_min "read_list read_var")

set(NAME_ID_occurrence "CT_OCCURRENCE")
set(NAME_TYPE_occurrence "STATIC_CT")
set(NAME_READ_occurrence "read_list read_constant read_var")

set(NAME_ID_occurrenceleq "CT_LEQ_OCCURRENCE")
set(NAME_TYPE_occurrenceleq "STATIC_CT")
set(NAME_READ_occurrenceleq "read_list read_constant read_constant")

set(NAME_ID_occurrencegeq "CT_GEQ_OCCURRENCE")
set(NAME_TYPE_occurrencegeq "STATIC_CT")
set(NAME_READ_occurrencegeq "read_list read_constant read_constant")

set(NAME_ID_product "CT_PRODUCT2")
set(NAME_TYPE_product "STATIC_CT")
set(NAME_READ_product "read_2_vars read_var")

set(NAME_ID_difference "CT_DIFFERENCE")
set(NAME_TYPE_difference "STATIC_CT")
set(NAME_READ_difference "read_2_vars read_var")

set(NAME_ID_weightedsumleq "CT_WEIGHTLEQSUM")
set(NAME_TYPE_weightedsumleq "STATIC_CT")
set(NAME_READ_weightedsumleq "read_constant_list read_list read_var")

set(NAME_ID_weightedsumgeq "CT_WEIGHTGEQSUM")
set(NAME_TYPE_weightedsumgeq "STATIC_CT")
set(NAME_READ_weightedsumgeq "read_constant_list read_list read_var")

set(NAME_ID_sumgeq "CT_GEQSUM")
set(NAME_TYPE_sumgeq "STATIC_CT")
set(NAME_READ_sumgeq "read_list read_var")

set(NAME_ID_sumleq "CT_LEQSUM")
set(NAME_TYPE_sumleq "STATIC_CT")
set(NAME_READ_sumleq "read_list read_var")

set(NAME_ID_watchsumgeq "CT_WATCHED_GEQSUM")
set(NAME_TYPE_watchsumgeq "DYNAMIC_CT")
set(NAME_READ_watchsumgeq "read_list read_constant")

set(NAME_ID_watchsumleq "CT_WATCHED_LEQSUM")
set(NAME_TYPE_watchsumleq "DYNAMIC_CT")
set(NAME_READ_watchsumleq "read_list read_constant")

set(NAME_ID_table "CT_WATCHED_TABLE")
set(NAME_TYPE_table "DYNAMIC_CT")
set(NAME_READ_table "read_list read_tuples")

set(NAME_ID_negativetable "CT_WATCHED_NEGATIVE_TABLE")
set(NAME_TYPE_negativetable "DYNAMIC_CT")
set(NAME_READ_negativetable "read_list read_tuples")

set(NAME_ID_watchvecneq "CT_WATCHED_VECNEQ")
set(NAME_TYPE_watchvecneq "DYNAMIC_CT")
set(NAME_READ_watchvecneq "read_list read_list")

set(NAME_ID_litsumgeq "CT_WATCHED_LITSUM")
set(NAME_TYPE_litsumgeq "DYNAMIC_CT")
set(NAME_READ_litsumgeq "read_list read_constant_list read_constant")

set(NAME_ID_pow "CT_POW")
set(NAME_TYPE_pow "STATIC_CT")
set(NAME_READ_pow "read_2_vars read_var")

set(NAME_ID_div "CT_DIV")
set(NAME_TYPE_div "STATIC_CT")
set(NAME_READ_div "read_2_vars read_var")

set(NAME_ID_modulo "CT_MODULO")
set(NAME_TYPE_modulo "STATIC_CT")
set(NAME_READ_modulo "read_2_vars read_var")

set(NAME_ID_gadget "CT_GADGET")
set(NAME_TYPE_gadget "STATIC_CT")
set(NAME_READ_gadget "read_list")

set(NAME_ID_disabled-or "CT_WATCHED_OR")
set(NAME_TYPE_disabled-or "DYNAMIC_CT")
set(NAME_READ_disabled-or "read_list")

set(NAME_ID_watchvecexists_less "CT_WATCHED_VEC_OR_LESS")
set(NAME_TYPE_watchvecexists_less "DYNAMIC_CT")
set(NAME_READ_watchvecexists_less "read_list read_list")

set(NAME_ID_watchvecexists_and "CT_WATCHED_VEC_OR_AND")
set(NAME_TYPE_watchvecexists_and "DYNAMIC_CT")
set(NAME_READ_watchvecexists_and "read_list read_list")

set(NAME_ID_hamming "CT_WATCHED_HAMMING")
set(NAME_TYPE_hamming "DYNAMIC_CT")
set(NAME_READ_hamming "read_list read_list read_constant")

set(NAME_ID_watched-or "CT_WATCHED_NEW_OR")
set(NAME_TYPE_watched-or "DYNAMIC_CT")
set(NAME_READ_watched-or "read_constraint_list")

set(NAME_ID_watched-and "CT_WATCHED_NEW_AND")
set(NAME_TYPE_watched-and "DYNAMIC_CT")
set(NAME_READ_watched-and "read_constraint_list")

set(NAME_ID_w-inset "CT_WATCHED_INSET")
set(NAME_TYPE_w-inset "DYNAMIC_CT")
set(NAME_READ_w-inset "read_var read_constant_list")

set(NAME_ID_w-notinset "CT_WATCHED_NOT_INSET")
set(NAME_TYPE_w-notinset "DYNAMIC_CT")
set(NAME_READ_w-notinset "read_var read_constant_list")

set(NAME_ID_w-inrange "CT_WATCHED_INRANGE")
set(NAME_TYPE_w-inrange "DYNAMIC_CT")
set(NAME_READ_w-inrange "read_var read_constant_list")

set(NAME_ID_w-notinrange "CT_WATCHED_NOT_INRANGE")
set(NAME_TYPE_w-notinrange "DYNAMIC_CT")
set(NAME_READ_w-notinrange "read_var read_constant_list")

set(NAME_ID_w-literal "CT_WATCHED_LIT")
set(NAME_TYPE_w-literal "DYNAMIC_CT")
set(NAME_READ_w-literal "read_var read_constant")

set(NAME_ID_w-notliteral "CT_WATCHED_NOTLIT")
set(NAME_TYPE_w-notliteral "DYNAMIC_CT")
set(NAME_READ_w-notliteral "read_var read_constant")

set(NAME_ID_reify "CT_REIFY")
set(NAME_TYPE_reify "DYNAMIC_CT")
set(NAME_READ_reify "read_constraint read_bool_var")

set(NAME_ID_reifyimply-quick "CT_REIFYIMPLY_QUICK")
set(NAME_TYPE_reifyimply-quick "DYNAMIC_CT")
set(NAME_READ_reifyimply-quick "read_constraint read_bool_var")

set(NAME_ID_reifyimply "CT_REIFYIMPLY")
set(NAME_TYPE_reifyimply "DYNAMIC_CT")
set(NAME_READ_reifyimply "read_constraint read_bool_var")

macro(select_constraints)
    set(internal-constraint-list "")
    foreach(constraint ${ARGV})
        set(index -1)
        list(FIND ALL_CONSTRAINTS ${constraint} index)
        if(${index} GREATER -1)
            list(APPEND internal-constraint-list
                 "${NAME_TYPE_${constraint}} \"${constraint}\" ${NAME_ID_${constraint}} ${NAME_READ_${constraint}}")
        endif()
    endforeach()
    file(REMOVE "minion/build_constraints/ConstraintList")
    foreach(constraint ${internal-constraint-list})
        file(APPEND "minion/build_constraints/ConstraintList" "${constraint}\n")
    endforeach()
endmacro()