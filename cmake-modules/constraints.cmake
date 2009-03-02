set(ALL_CONSTRAINTS "element" "element_one" "watchelement" "watchelement_one"
                    "gacelement-deprecated" "alldiff" "gacalldiff" "gcc" "gccweak" "watchneq"
                    "diseq" "__reify_diseq" "eq" "__reify_eq" "minuseq" "__reify_minuseq"
                    "abs" "ineq" "watchless" "lexleq" "lexless" "max" "min" "occurrence"
                    "occurrenceleq" "occurrencegeq" "product" "difference"
                    "weightedsumleq" "weightedsumgeq" "sumgeq" "sumleq" "watchsumgeq"
                    "watchsumleq" "table" "negativetable" "watchvecneq" "litsumgeq"
                    "pow" "div" "modulo" "gadget" "disabled-or"
                    "hamming" "watched-or" "watched-and"
                    "w-inset" "w-notinset" "w-inrange" "w-notinrange" "w-literal"
                    "w-notliteral" "reify" "reifyimply-quick" "reifyimply" "reifyimply-old" "reifyimply-new" "true" "false")

set(GEN_FILES_DIR "${PROJECT_SOURCE_DIR}/minion/build_constraints")
set(CONSTRAINTS_LIST "${GEN_FILES_DIR}/ConstraintList")
set(CONSTRAINT_DEFS "${GEN_FILES_DIR}/constraint_defs.h")
set(CONSTRAINT_ENUM "${GEN_FILES_DIR}/ConstraintEnum.h")
set(BUILD_CONSTRAINTS_START "${GEN_FILES_DIR}/BuildConstraintsStart.h")
set(BUILD_START "${GEN_FILES_DIR}/BuildStart.h")
set(BUILD_STATIC_START "${GEN_FILES_DIR}/BuildStaticStart.cpp")
set(CONSTRAINT_HEADER "#include \"../minion.h\"
/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 
*/\n\n")

set(NAME_ID_element "CT_ELEMENT")
set(NAME_READ_element "read_list" "read_var" "read_var")

set(NAME_ID_element_one "CT_ELEMENT_ONE")
set(NAME_READ_element_one "read_list" "read_var" "read_var")

set(NAME_ID_watchelement "CT_WATCHED_ELEMENT")
set(NAME_READ_watchelement "read_list" "read_var" "read_var")

set(NAME_ID_watchelement_one "CT_WATCHED_ELEMENT_ONE")
set(NAME_READ_watchelement_one "read_list" "read_var" "read_var")

set(NAME_ID_gacelement-deprecated "CT_GACELEMENT")
set(NAME_READ_gacelement-deprecated "read_list" "read_var" "read_var")

set(NAME_ID_alldiff "CT_ALLDIFF")
set(NAME_READ_alldiff "read_list")

set(NAME_ID_gacalldiff "CT_GACALLDIFF")
set(NAME_READ_gacalldiff "read_list")

set(NAME_ID_gcc "CT_GCC")
set(NAME_READ_gcc "read_list" "read_constant_list" "read_list")

set(NAME_ID_gccweak "CT_GCCWEAK")
set(NAME_READ_gccweak "read_list" "read_constant_list" "read_list")

set(NAME_ID_watchneq "CT_WATCHED_NEQ")
set(NAME_READ_watchneq "read_var" "read_var")

set(NAME_ID_diseq "CT_DISEQ")
set(NAME_READ_diseq "read_var" "read_var")

set(NAME_ID___reify_diseq "CT_DISEQ_REIFY")
set(NAME_READ___reify_diseq "read_var" "read_var" "read_bool_var")

set(NAME_ID_eq "CT_EQ")
set(NAME_READ_eq "read_var" "read_var")

set(NAME_ID___reify_eq "CT_EQ_REIFY")
set(NAME_READ___reify_eq "read_var" "read_var" "read_bool_var")

set(NAME_ID_minuseq "CT_MINUSEQ")
set(NAME_READ_minuseq "read_var" "read_var")

set(NAME_ID___reify_minuseq "CT_MINUSEQ_REIFY")
set(NAME_READ___reify_minuseq "read_var" "read_var" "read_bool_var")

set(NAME_ID_abs "CT_ABS")
set(NAME_READ_abs "read_var" "read_var")

set(NAME_ID_ineq "CT_INEQ")
set(NAME_READ_ineq "read_var" "read_var" "read_constant")

set(NAME_ID_watchless "CT_WATCHED_LESS")
set(NAME_READ_watchless "read_var" "read_var")

set(NAME_ID_lexleq "CT_LEXLEQ")
set(NAME_READ_lexleq "read_list" "read_list")

set(NAME_ID_lexless "CT_LEXLESS")
set(NAME_READ_lexless "read_list" "read_list")

set(NAME_ID_max "CT_MAX")
set(NAME_READ_max "read_list" "read_var")

set(NAME_ID_min "CT_MIN")
set(NAME_READ_min "read_list" "read_var")

set(NAME_ID_occurrence "CT_OCCURRENCE")
set(NAME_READ_occurrence "read_list" "read_constant" "read_var")

set(NAME_ID_occurrenceleq "CT_LEQ_OCCURRENCE")
set(NAME_READ_occurrenceleq "read_list" "read_constant" "read_constant")

set(NAME_ID_occurrencegeq "CT_GEQ_OCCURRENCE")
set(NAME_READ_occurrencegeq "read_list" "read_constant" "read_constant")

set(NAME_ID_product "CT_PRODUCT2")
set(NAME_READ_product "read_2_vars" "read_var")

set(NAME_ID_difference "CT_DIFFERENCE")
set(NAME_READ_difference "read_2_vars" "read_var")

set(NAME_ID_weightedsumleq "CT_WEIGHTLEQSUM")
set(NAME_READ_weightedsumleq "read_constant_list" "read_list" "read_var")

set(NAME_ID_weightedsumgeq "CT_WEIGHTGEQSUM")
set(NAME_READ_weightedsumgeq "read_constant_list" "read_list" "read_var")

set(NAME_ID_sumgeq "CT_GEQSUM")
set(NAME_READ_sumgeq "read_list" "read_var")

set(NAME_ID_sumleq "CT_LEQSUM")
set(NAME_READ_sumleq "read_list" "read_var")

set(NAME_ID_watchsumgeq "CT_WATCHED_GEQSUM")
set(NAME_READ_watchsumgeq "read_list" "read_constant")

set(NAME_ID_watchsumleq "CT_WATCHED_LEQSUM")
set(NAME_READ_watchsumleq "read_list" "read_constant")

set(NAME_ID_table "CT_WATCHED_TABLE")
set(NAME_READ_table "read_list" "read_tuples")

set(NAME_ID_negativetable "CT_WATCHED_NEGATIVE_TABLE")
set(NAME_READ_negativetable "read_list" "read_tuples")

set(NAME_ID_watchvecneq "CT_WATCHED_VECNEQ")
set(NAME_READ_watchvecneq "read_list" "read_list")

set(NAME_ID_litsumgeq "CT_WATCHED_LITSUM")
set(NAME_READ_litsumgeq "read_list" "read_constant_list" "read_constant")

set(NAME_ID_pow "CT_POW")
set(NAME_READ_pow "read_2_vars" "read_var")

set(NAME_ID_div "CT_DIV")
set(NAME_READ_div "read_2_vars" "read_var")

set(NAME_ID_modulo "CT_MODULO")
set(NAME_READ_modulo "read_2_vars" "read_var")

set(NAME_ID_gadget "CT_GADGET")
set(NAME_READ_gadget "read_list")

set(NAME_ID_disabled-or "CT_WATCHED_OR")
set(NAME_READ_disabled-or "read_list")

set(NAME_ID_hamming "CT_WATCHED_HAMMING")
set(NAME_READ_hamming "read_list" "read_list" "read_constant")

set(NAME_ID_watched-or "CT_WATCHED_NEW_OR")
set(NAME_READ_watched-or "read_constraint_list")

set(NAME_ID_watched-and "CT_WATCHED_NEW_AND")
set(NAME_READ_watched-and "read_constraint_list")

set(NAME_ID_w-inset "CT_WATCHED_INSET")
set(NAME_READ_w-inset "read_var" "read_constant_list")

set(NAME_ID_w-notinset "CT_WATCHED_NOT_INSET")
set(NAME_READ_w-notinset "read_var" "read_constant_list")

set(NAME_ID_w-inrange "CT_WATCHED_INRANGE")
set(NAME_READ_w-inrange "read_var" "read_constant_list")

set(NAME_ID_w-notinrange "CT_WATCHED_NOT_INRANGE")
set(NAME_READ_w-notinrange "read_var" "read_constant_list")

set(NAME_ID_w-literal "CT_WATCHED_LIT")
set(NAME_READ_w-literal "read_var" "read_constant")

set(NAME_ID_w-notliteral "CT_WATCHED_NOTLIT")
set(NAME_READ_w-notliteral "read_var" "read_constant")

set(NAME_ID_reify "CT_REIFY")
set(NAME_READ_reify "read_constraint" "read_bool_var")

set(NAME_ID_reifyimply-quick "CT_REIFYIMPLY_QUICK")
set(NAME_READ_reifyimply-quick "read_constraint" "read_bool_var")

set(NAME_ID_reifyimply "CT_REIFYIMPLY")
set(NAME_READ_reifyimply "read_constraint" "read_bool_var")

set(NAME_ID_reifyimply-old "CT_REIFYIMPLY_OLD")
set(NAME_READ_reifyimply-old "read_constraint" "read_bool_var")

set(NAME_ID_reifyimply-new "CT_REIFYIMPLY_NEW")
set(NAME_READ_reifyimply-new "read_constraint" "read_bool_var")

set(NAME_ID_true "CT_TRUE")
set(NAME_READ_true )

set(NAME_ID_false "CT_FALSE")
set(NAME_READ_false )

macro(select_constraints)
    message(STATUS "Generating constraints:")
    file(GLOB CONSTRAINT_SRCS "${GEN_FILES_DIR}/CT*.cpp")
    list(LENGTH CONSTRAINT_SRCS CONSTRAINT_SRCS_NUM)
    if(${CONSTRAINT_SRCS_NUM} GREATER 0)
        file(REMOVE ${CONSTRAINT_SRCS})
    endif()
    file(REMOVE ${CONSTRAINTS_LIST})
    file(REMOVE ${CONSTRAINT_DEFS})
    file(REMOVE ${CONSTRAINT_ENUM})
    file(REMOVE ${BUILD_CONSTRAINTS_START})
    file(REMOVE ${BUILD_START})
    file(REMOVE ${BUILD_STATIC_START})
    file(APPEND ${CONSTRAINT_DEFS} "ConstraintDef constraint_list[] = {\n")
    file(APPEND ${CONSTRAINT_ENUM} "#ifndef CONSTRAINT_ENUM_H_BLARG\n")
    file(APPEND ${CONSTRAINT_ENUM} "#define CONSTRAINT_ENUM_H_BLARG\n")
    file(APPEND ${CONSTRAINT_ENUM} "enum ConstraintType {\n")
    file(APPEND ${BUILD_START} "#define NO_MAIN\n#include \"../minion.h\"\n")
    file(APPEND ${BUILD_STATIC_START} "#include \"BuildStart.h\"\n")
    file(APPEND ${BUILD_STATIC_START} "AbstractConstraint* build_constraint(StateObj* stateObj, ConstraintBlob& b) {\n")
    file(APPEND ${BUILD_STATIC_START} "switch(b.constraint->type) {\n")
    file(GLOB_RECURSE constraint_headers
                      RELATIVE "${GEN_FILES_DIR}"
                      "${PROJECT_SOURCE_DIR}/minion/constraints/*.h"
                      "${PROJECT_SOURCE_DIR}/minion/dynamic_constraints/*.h")
    set(msg "")
    foreach(constraint ${ARGV})
        set(found False)
        foreach(defined_constraint ${ALL_CONSTRAINTS})
            if(${defined_constraint} STREQUAL ${constraint})
                set(found True)
            endif()
        endforeach()
        if(found)
            set(msg "${msg} ${constraint}")
            string(LENGTH ${msg} msglength)
            if(${msglength} GREATER 60)
                message(STATUS "${msg}")
                set(msg "")
            endif()
            list(LENGTH NAME_READ_${constraint} num_read_funcs)
            set(build_read_funcs "")
            foreach(read_func ${NAME_READ_${constraint}})
                if(${read_func} MATCHES "read_list|read_var|read_bool_var|read_2_vars")
                    list(APPEND build_read_funcs ${read_func})
                endif()
            endforeach()
            list(LENGTH build_read_funcs build_num_read_funcs)
            # constraint_defs.h
            file(APPEND ${CONSTRAINT_DEFS} "{ \"${constraint}\", ${NAME_ID_${constraint}}, ${num_read_funcs}, {")
            foreach(read_func ${NAME_READ_${constraint}})
                file(APPEND ${CONSTRAINT_DEFS} "${read_func}, ")
            endforeach()
            file(APPEND ${CONSTRAINT_DEFS} "}, },\n")
            # ConstraintEnum.h
            file(APPEND ${CONSTRAINT_ENUM} "${NAME_ID_${constraint}},\n")
            # BuildConstraintStart.h
            file(APPEND ${BUILD_CONSTRAINTS_START} "BUILD_DEF ## ${build_num_read_funcs}(${NAME_ID_${constraint}})\n")
            # BuildStart.h
            file(APPEND ${BUILD_START} "BUILD_DEF_CT(${NAME_ID_${constraint}})\n")
            # BuildStaticStart.h
            file(APPEND ${BUILD_STATIC_START} "case ${NAME_ID_${constraint}}: return build_constraint_${NAME_ID_${constraint}}(stateObj, b);\n")
            # CT_*.cpp
            file(REMOVE "${GEN_FILES_DIR}/${NAME_ID_${constraint}}.cpp")
            file(APPEND "${GEN_FILES_DIR}/${NAME_ID_${constraint}}.cpp"
                        ${CONSTRAINT_HEADER})
            foreach(constraint_header ${constraint_headers})
                file(READ "${GEN_FILES_DIR}/${constraint_header}" contents)
                if(contents MATCHES "${NAME_ID_${constraint}}")
                    file(APPEND "${GEN_FILES_DIR}/${NAME_ID_${constraint}}.cpp"
                                "#include \"${constraint_header}\"\n")
                endif()
            endforeach()
            file(APPEND "${GEN_FILES_DIR}/${NAME_ID_${constraint}}.cpp"
                        "\nBUILD_CT(${NAME_ID_${constraint}}, ${build_num_read_funcs})\n")
        endif()
    endforeach()
    message(STATUS "${msg}")
    file(APPEND ${CONSTRAINT_DEFS} "};")
    file(APPEND ${CONSTRAINT_ENUM} "};\n")
    file(APPEND ${CONSTRAINT_ENUM} "#endif")
    file(APPEND ${BUILD_STATIC_START} "default: D_FATAL_ERROR(\"Fatal error building constraints\");\n")
    file(APPEND ${BUILD_STATIC_START} "}}")
endmacro(select_constraints)
