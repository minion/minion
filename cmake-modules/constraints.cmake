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

macro(select_constraints)
    set(internal-constraint-list "")
    foreach(constraint ${ARGV})
        if(${constraint} MATCHES "([^a-z_-]|^)element([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"element\" CT_ELEMENT read_list read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)element_one([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"element_one\" CT_ELEMENT_ONE read_list read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchelement([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchelement\" CT_WATCHED_ELEMENT read_list read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchelement_one([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchelement_one\" CT_WATCHED_ELEMENT_ONE read_list read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)gacelement-deprecated([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"gacelement-deprecated\" CT_GACELEMENT read_list read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)alldiff([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"alldiff\" CT_ALLDIFF read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)gacalldiff([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"gacalldiff\" CT_GACALLDIFF read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)gcc([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"gcc\" CT_GCC read_list read_constant_list read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchneq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchneq\" CT_WATCHED_NEQ read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)diseq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"diseq\" CT_DISEQ read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)__reify_diseq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"__reify_diseq\" CT_DISEQ_REIFY read_var read_var read_bool_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)eq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"eq\" CT_EQ read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)__reify_eq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"__reify_eq\" CT_EQ_REIFY read_var read_var read_bool_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)minuseq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"minuseq\" CT_MINUSEQ read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)__reify_minuseq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"__reify_minuseq\" CT_MINUSEQ_REIFY read_var read_var read_bool_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)abs([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"abs\" CT_ABS read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)ineq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"ineq\" CT_INEQ read_var read_var read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchless([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchless\" CT_WATCHED_LESS read_var read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)lexleq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"lexleq\" CT_LEXLEQ read_list read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)lexless([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"lexless\" CT_LEXLESS read_list read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)max([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"max\" CT_MAX read_list read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)min([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"min\" CT_MIN read_list read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)occurrence([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"occurrence\" CT_OCCURRENCE read_list read_constant read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)occurrenceleq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"occurrenceleq\" CT_LEQ_OCCURRENCE read_list read_constant read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)occurrencegeq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"occurrencegeq\" CT_GEQ_OCCURRENCE read_list read_constant read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)product([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"product\" CT_PRODUCT2 read_2_vars read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)difference([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"difference\" CT_DIFFERENCE read_2_vars read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)weightedsumleq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"weightedsumleq\" CT_WEIGHTLEQSUM read_constant_list read_list read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)weightedsumgeq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"weightedsumgeq\" CT_WEIGHTGEQSUM read_constant_list read_list read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)sumgeq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"sumgeq\" CT_GEQSUM read_list read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)sumleq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"sumleq\" CT_LEQSUM read_list read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchsumgeq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchsumgeq\" CT_WATCHED_GEQSUM read_list read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchsumleq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchsumleq\" CT_WATCHED_LEQSUM read_list read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)table([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"table\" CT_WATCHED_TABLE read_list read_tuples")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)negativetable([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"negativetable\" CT_WATCHED_NEGATIVE_TABLE read_list read_tuples")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchvecneq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchvecneq\" CT_WATCHED_VECNEQ read_list read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)litsumgeq([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"litsumgeq\" CT_WATCHED_LITSUM read_list read_constant_list read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)pow([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"pow\" CT_POW read_2_vars read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)div([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"div\" CT_DIV read_2_vars read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)modulo([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"modulo\" CT_MODULO read_2_vars read_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)gadget([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "STATIC_CT \"gadget\" CT_GADGET read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)disabled-or([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"disabled-or\" CT_WATCHED_OR read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchvecexists_less([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchvecexists_less\" CT_WATCHED_VEC_OR_LESS read_list read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watchvecexists_and([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watchvecexists_and\" CT_WATCHED_VEC_OR_AND read_list read_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)hamming([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"hamming\" CT_WATCHED_HAMMING read_list read_list read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watched-or([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watched-or\" CT_WATCHED_NEW_OR read_constraint_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)watched-and([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"watched-and\" CT_WATCHED_NEW_AND read_constraint_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)w-inset([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"w-inset\" CT_WATCHED_INSET read_var read_constant_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)w-notinset([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"w-notinset\" CT_WATCHED_NOT_INSET read_var read_constant_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)w-inrange([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"w-inrange\" CT_WATCHED_INRANGE read_var read_constant_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)w-notinrange([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"w-notinrange\" CT_WATCHED_NOT_INRANGE read_var read_constant_list")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)w-literal([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"w-literal\" CT_WATCHED_LIT read_var read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)w-notliteral([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"w-notliteral\" CT_WATCHED_NOTLIT read_var read_constant")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)reify([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"reify\" CT_REIFY read_constraint read_bool_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)reifyimply-quick([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"reifyimply-quick\" CT_REIFYIMPLY_QUICK read_constraint read_bool_var")
        endif()
        if(${constraint} MATCHES "([^a-z_-]|^)reifyimply([^a-z_-]|$)")
            list(APPEND internal-constraint-list
                 "DYNAMIC_CT \"reifyimply\" CT_REIFYIMPLY read_constraint read_bool_var")
        endif()
    endforeach()
    file(REMOVE "minion/build_constraints/ConstraintList")
    foreach(constraint ${internal-constraint-list})
        file(APPEND "minion/build_constraints/ConstraintList" "${constraint}\n")
    endforeach()
endmacro()
