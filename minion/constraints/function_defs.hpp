/*
 *  function_defs.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 25/03/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */

// This file contains all definitions which need to be after
// everything else. In an ideal world, this would live in a
// .c file, but we want them to be inlinable.

inline void Trigger::propogate(DomainDelta domain_data)
{
    D_INFO(0,DI_SOLVER,"Trigger Activated");
#ifdef FUNCTIONPOINTER_TRIGGER
    constraint_function_ptr(constraint, info, domain_data);
#else
    constraint->propogate(info, domain_data); 
#endif
}

inline void Trigger::full_propogate()
{
    D_INFO(0,DI_SOLVER,"Trigger Full Propogated");
    constraint->full_propogate(); 
}

inline Constraint* Constraint::get_table_constraint()
{ return new TableConstraint<vector<AnyVarRef> >(this->get_vars(), this); }
