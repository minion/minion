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

namespace Controller
{
    
/// Lists all structures that must be locked before search.
// @todo This could be done more neatly... 
  void lock(StateObj*);

} // namespace Controller

