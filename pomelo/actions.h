//
//  actions.h
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#ifndef ACTIONS_H
#define ACTIONS_H


#include "errors.h"
#include "automata.h"


/*
    Works out actions for each state, resolving and reporting conflicts.
*/

class actions
{
public:

    explicit actions( errors_ptr errors, automata_ptr automata );
    actions();
    
    void analyze();


private:

    struct context_production
    {
        symbol* symbol;
        std::vector< context_production > children;
    };
    
    typedef std::vector< context_production > context;

    void traverse_start( state* s, int distance );
    void left_context( state* s, context* out_lcontext );
    void traverse_accept( state* s, int distance );
    void right_context( state* s, context* out_rcontext );
    void reduce_context( rule* rule, context* out_rcontext );
    state* emulate_reduce( state* s, reduction* reduce, context* out_context );
    state* emulate( state* s, terminal* token, context* out_context );
    std::string print_context( const context& context );

    void build_actions( state* s );
    void report_conflicts( state* s );
    bool similar_conflict( conflict* a, conflict* b );
    void report_conflict( state* s, const std::vector< conflict* >& conflicts );

    errors_ptr _errors;
    automata_ptr _automata;
    bool _calculated_distances;

};



#endif


