//
//  automata.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 17/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "automata.h"



/*
    Deletes closure pointers using free().
*/

void closure_deleter::operator () ( closure* p ) const
{
    free( p );
}

