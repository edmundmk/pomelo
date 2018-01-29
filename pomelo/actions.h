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
    Calculates the context of a conflict by searching through the parse tree.
*/

class context
{
public:

    context( automata_ptr automata, state* left_context );
    ~context();
    
    void shift( state* next, terminal* term );
    void reduce( rule* rule, terminal* term );
    void search();
    void print( std::string* out_print );
    
    
private:

    /*
        To produce a meaningful example parse, we need to do a search through
        the space of possible parse trees.  A value represents a value on one
        of the potential parse stacks.
    */

    struct value
    {
        value( value* prev, state* state, symbol* symbol )
            :   prev( prev )
            ,   state( state )
            ,   symbol( symbol )
            ,   chead( nullptr )
            ,   ctail( nullptr )
        {
        }
    
        value* prev;        // previous value in stack.
        state* state;       // state parser was in when this value was pushed.
        symbol* symbol;     // symbol pushed onto stack.
        value* chead;       // first value in list of children in parse tree.
        value* ctail;       // last value in list of children in parse tree.
    };
    
    typedef std::unique_ptr< value > value_ptr;
    
    /*
        The shead structure represents a node in the search space which has not
        had all its successor states evaluated.
    */
    
    struct shead
    {
        shead( value* stack, state* state, int distance )
            :   stack( stack )
            ,   state( state )
            ,   distance( distance )
        {
        }
    
        value* stack;       // value on top of stack.
        state* state;       // state parse is in.
        int distance;       // number of actions to get to this state.
    };
    

    /*
        The search heuristic is the number of symbols shifted plus the shortest
        distance through the DFA from the current state to the
    */
    
    struct heuristic
    {
        bool operator () ( const shead& a, const shead& b ) const;
    };

    
    shead shift( const shead& head, state* next, symbol* sym );
    shead reduce( const shead& head, rule* rule );
    void parse( const shead& head, terminal* term );
    void print( const value* tail, const value* head, std::string* out_print );


    automata_ptr _automata;
    shead _left;
    shead _tree;
    std::vector< value_ptr > _values;
    std::priority_queue< shead, std::vector< shead >, heuristic > _open;
    
};



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


