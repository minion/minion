/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id$
*/

/*
 *  constraint_setup.h
 *  cutecsp
 *
 *  Created by Chris Jefferson on 17/05/2006.
 *  Copyright 2006 __MyCompanyName__. All rights reserved.
 *
 */
 
#ifndef _CONSTRAINT_SETUP_H
#define _CONSTRAINT_SETUP_H 

// The following is a little trick, to make sure no-one accidentally links together
// debugging and non-debugging code (which are not link-compatable)
#ifdef MINION_DEBUG
namespace StateObjNamespace_DEBUG
#else
namespace StateObjNamespace_RELEASE
#endif
{ struct StateObj; }
 
#ifdef MINION_DEBUG
using namespace StateObjNamespace_DEBUG;
#else
using namespace StateObjNamespace_RELEASE;
#endif

namespace Controller
{
    
/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 
  void lock(StateObj*);

} // namespace Controller

#endif
