//
//  conflicts.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include "conflicts.h"



conflicts::conflicts( errors_ptr errors, automata_ptr automata )
    :   _errors( errors )
    ,   _automata( automata )
{
}

conflicts::~conflicts()
{
}


void conflicts::analyze()
{
    // Traverse DFA from start state to work out distance from the start.
    traverse_start( _automata->start, 0 );
    
    // Traverse DFA from accept state (and following rfrom links) to work
    // out distance to the accept state.
    traverse_accept( _automata->accept, 0 );


}


void conflicts::traverse_start( state* state, int distance )
{
    
}

void conflicts::traverse_accept( state* state, int distance )
{
}


