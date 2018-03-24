//
//  search.h
//  pomelo
//
//  Created by Edmund Kapusniak on 29/01/2018.
//  Copyright © 2018 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#ifndef EXAMPLE_H
#define EXAMPLE_H


#include <queue>
#include "automata.h"


struct left_context;
class left_search;
class parse_search;

typedef std::shared_ptr< left_context > left_context_ptr;
typedef std::shared_ptr< left_search > left_search_ptr;
typedef std::shared_ptr< parse_search > parse_search_ptr;



/*
    To print a meaningful example of a conflict, we need to build parse trees
    exhibiting the results of the actions that the parser could take.  This
    requires pathfinding through the DFA.
*/



/*
    A left context is a chain of symbols that takes us from the start state
    to the target state.  The shortest left context might not allow the
    reductions required to demonstrate the ambiguity, so we might need to
    generate longer ones.
*/

struct left_context_value
{
    state* xstate;
    symbol* sym;
};

struct left_context
{
    state* xstate;
    std::vector< left_context_value > context;
};




/*
    Produces progressively more complex left contexts that result in state.
    Performs a depth-first search backwards through the DFA.  Eventually all
    routes through the DFA will be exhausted.
*/

class left_search
{
public:

    left_search( automata_ptr automata, state* target );
    ~left_search();
    
    /*
        Generates another unique left context that can be used to build
        example parse trees.
    */
    left_context_ptr generate();
    
    
private:
    
    /*
        A search node.  From the linked chain of search nodes we can recover
        the chain of symbols that take us from the start to the target state.
    */
    
    struct left_node;
    typedef std::unique_ptr< left_node > left_node_ptr;
    typedef std::vector< left_node_ptr > left_node_list;
    
    struct left_node
    {
        explicit left_node( state* xstate );
        left_node( left_node* parent, transition* trans );
    
        left_node* parent;      // parent search node (closer to target state).
        left_node_list child;   // search nodes for each transition leaving state.
        state* xstate;          // state the search has reached.
        symbol* sym;            // symbol that transitioned from previous.
        int distance;           // distance from the target state.
    };
    
    struct heuristic
    {
        bool operator () ( left_node* a, left_node* b ) const;
    };
    
    left_context_ptr generate_route( left_node* node );
    
    automata_ptr _automata;
    state* _target;
    left_node_ptr _root;
    std::priority_queue< left_node*, std::vector< left_node* >, heuristic > _open;
    
};



/*
    Produces an example parse tree exibiting a particular parser action, given
    a particular left context.  If the action cannot be performed then the user
    can retry with a new left context.
 
    If the action succeeds in the immediate context, then it proceeds to search
    through the actions in the DFA until it finds an accept state (i.e. a valid
    parse).  It's possible that all paths fail, in which case it will indicate
    this in the result.
*/

class parse_search
{
public:

    parse_search( automata_ptr automata, left_context_ptr lcontext );
    ~parse_search();

    bool shift( state* next, terminal* term );
    bool reduce( rule* rule, terminal* term );
    bool search();
    void print( std::string* out_print );


private:

    struct value;
    typedef std::unique_ptr< value > value_ptr;


    /*
        To produce a meaningful example parse, we need to do a search through
        the space of possible parse trees.  A value represents a value on one
        of the potential parse stacks.
    */

    struct value
    {
        value( value* prev, state* xstate, symbol* sym );
        
        value* prev;    // previous value in stack.
        state* xstate;  // state parser was in when this value was pushed.
        symbol* sym;    // symbol pushed onto stack.
        value* chead;   // first value in list of children in parse tree.
        value* ctail;   // last value in list of children in parse tree.
    };
    
    
    /*
        This structure represents a node in the search space which has not
        had all its successor states evaluated.
    */

    struct search_head
    {
        search_head( value* stack, state* xstate, int distance );
    
        value* stack;   // value on top of stack.
        state* xstate;  // state parse is in.
        int distance;   // number of actions to get to this state.
    };
    
    
    /*
        The search heuristic is the number of symbols shifted plus the shortest
        distance through the DFA from the current state to the accept state.
    */
    
    struct heuristic
    {
        bool operator () ( const search_head& a, const search_head& b ) const;
    };

    
    void parse( const search_head& head, terminal* term );
    search_head shift( const search_head& head, state* next, symbol* sym );
    search_head reduce( const search_head& head, rule* rule );
    
    void print( const value* tail, const value* head, std::string* out_print );
    

    automata_ptr _automata;
    std::vector< value_ptr > _values;
    search_head _left;
    value* _accept;
    std::priority_queue< search_head, std::vector< search_head >, heuristic > _open;

};








#endif
