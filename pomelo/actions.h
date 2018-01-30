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
typedef std::shared_ptr< actions > actions_ptr;



/*
    The action table is the main parser table.  It maps ( state, terminal ) to
    the action - error, shift, reduce, or conflict - to perform.
*/

struct action_conflict
{
    std::vector< int > actions;
};

struct action_table
{
    int max_shift;      // 0 <= n < max_shift -> shift terminal n
    int max_reduce;     // max_shift <= n < max_reduce -> reduce rule n - max_shift
    int max_conflict;   // max_reduce <= n < max_conflict -> conflict n
    int error;          // n == error -> error

    int terminal_count;
    int state_count;

    std::vector< terminal* > terminals;
    std::vector< rule* > rules;
    std::vector< action_conflict > conflicts;
    std::vector< int > actions;

    int& lookup( int state, int terminal );
};


/*
    The goto table indicates which state to transition to after a nonterminal
    is reduced.  It maps ( state, nonterminal ) to the new state.
*/

struct goto_table
{
    int nonterminal_count;
    int state_count;
    
    std::vector< int > gotos;
    
    int& lookup( int state, int nonterminal );
};



/*
    Works out actions for each state, resolving and reporting conflicts.
*/

class actions
{
public:

    actions( errors_ptr errors, automata_ptr automata );
    actions();
    
    void analyze();
    void report_conflicts();
    void print();


private:

    void build_actions( state* s );
    int rule_precedence( rule* r );
    
    void report_conflicts( state* s );
    bool similar_conflict( conflict* a, conflict* b );
    void report_conflict( state* s, const std::vector< conflict* >& conflicts );

    errors_ptr _errors;
    automata_ptr _automata;

};



#endif


