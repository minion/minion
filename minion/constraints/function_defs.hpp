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

inline void Trigger::propagate(DomainDelta domain_data)
{
    constraint->propagate(info, domain_data); 
}

inline void Trigger::full_propagate()
{
    constraint->full_propagate(); 
}
