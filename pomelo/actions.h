//
//  actions.h
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#ifndef ACTIONS_H
#define ACTIONS_H


#include <queue>
#include "errors.h"
#include "automata.h"


class actions;
struct action_table;
struct goto_table;
typedef std::shared_ptr< actions > actions_ptr;
typedef std::shared_ptr< action_table > action_table_ptr;
typedef std::shared_ptr< goto_table > goto_table_ptr;



/*
    The action table is the main parser table.  It maps ( state, terminal ) to
    the action - error, shift, reduce, or conflict - to perform.
*/

struct action_table
{
    int error_action;   // error action
    int accept_action;  // accept action

    int rule_count;     // max_state <= n < max_reduce -> reduce rule n - max_state
    int conflict_count; // max_reduce <= n < max_conflict -> conflict n - max_reduce
    
    std::vector< rule* > rules;
    std::vector< int > conflicts;
    
    int token_count;
    int state_count;

    std::vector< int > actions;
};


/*
    The goto table indicates which state to transition to after a nonterminal is
    reduced.  It maps ( state, nonterminal ) to the new state.
*/

struct goto_table
{
    int token_count;    // table is looked up with nonterminal index - token_count
    int nterm_count;
    int state_count;
    
    std::vector< int > gotos;
};



/*
    Works out actions for each state, resolving and reporting conflicts.
*/

class actions
{
public:

    actions( errors_ptr errors, automata_ptr automata, bool expected_info );
    actions();
    
    void analyze();
    action_table_ptr build_action_table();
    goto_table_ptr build_goto_table();
    void report_conflicts();
    void print();


private:

    void build_actions( state* s );
    int rule_precedence( rule* r );
    srcloc rule_location( rule* r );
    
    void traverse_rules( state* s );
    void traverse_reduce( state* s, reduction* reduce );
    
    int conflict_actval( action_table* table, conflict* conflict );
    
    void report_conflicts( state* s );
    bool similar_conflict( conflict* a, conflict* b );
    void report_conflict( state* s, const std::vector< conflict* >& conflicts );

    errors_ptr _errors;
    automata_ptr _automata;
    bool _expected_info;

};



#endif


