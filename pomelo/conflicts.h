//
//  conflicts.h
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#ifndef CONFLICTS_H
#define CONFLICTS_H


#include "errors.h"
#include "automata.h"


/*
    Resolve conflicts using precedence rules, otherwise report them.
*/

class conflicts
{
public:

    explicit conflicts( errors_ptr errors, automata_ptr automata );
    ~conflicts();
    
    void analyze();


private:

    void traverse_start( state* state, int distance );
    void traverse_accept( state* state, int distance );

    errors_ptr _errors;
    automata_ptr _automata;

};



#endif


