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


    void traverse_start( state* s, int distance );
    void traverse_accept( state* s, int distance );

    void build_actions( state* s );
    void report_conflicts( state* s );
    bool similar_conflict( conflict* a, conflict* b );
    void report_conflict( state* s, const std::vector< conflict* >& conflicts );

    errors_ptr _errors;
    automata_ptr _automata;
    bool _calculated_distances;

};



#endif


