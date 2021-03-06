//
//  automata.hpp
//  pomelo
//
//  Created by Edmund Kapusniak on 17/12/2017.
//  Copyright © 2017 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#ifndef AUTOMATA_H
#define AUTOMATA_H


#include <memory>
#include <vector>
#include "syntax.h"


struct symbol;
struct closure;
struct closure_deleter;
struct automata;
struct conflict;
struct action;
struct state;
struct transition;
struct reducefrom;
struct reduction;


typedef std::unique_ptr< closure, closure_deleter > closure_ptr;
typedef std::shared_ptr< automata > automata_ptr;
typedef std::unique_ptr< state > state_ptr;
typedef std::unique_ptr< conflict > conflict_ptr;
typedef std::unique_ptr< transition > transition_ptr;
typedef std::unique_ptr< reducefrom > reducefrom_ptr;
typedef std::unique_ptr< reduction > reduction_ptr;



/*
    A closure of locations in the grammar.
*/

struct closure
{
    size_t hash;
    size_t size;
    size_t locations[];
};

struct closure_deleter
{
    void operator () ( closure* p ) const;
};




/*
    A discrete finite automata.
*/

struct automata
{
    explicit automata( syntax_ptr syntax );
    ~automata();

    void ensure_distances();
    void print_graph( bool rgoto );
    void print_dump();

    syntax_ptr syntax;
    state* start;
    state* accept;
    std::vector< state_ptr > states;
    std::vector< transition_ptr > transitions;
    std::vector< reducefrom_ptr > reducefroms;
    std::vector< reduction_ptr > reductions;
    std::vector< conflict_ptr > conflicts;
    uintptr_t visited;
    int rule_count;
    int state_count;
    bool has_distances;
};

struct conflict
{
    explicit conflict( terminal* term );

    terminal* term;
    transition* shift;
    std::vector< reduction* > reduce;
    bool reported;
};

enum action_kind { ACTION_ERROR, ACTION_SHIFT, ACTION_REDUCE, ACTION_CONFLICT };

struct action
{
    action_kind kind;
    union
    {
        transition* shift;
        reduction*  reduce;
        conflict*   cflict;
    };
};

struct state
{
    explicit state( closure_ptr&& closure );

    closure_ptr closure;
    std::vector< action > actions;
    std::vector< reduction* > reductions;
    std::vector< transition* > prev;
    std::vector< transition* > next;
    uintptr_t visited;
    int start_distance;
    int accept_distance;
    int index;
    bool has_conflict;
    bool reachable;
};

struct transition
{
    transition( state* prev, state* next, symbol* nsym, token tok, bool conflicts );

    state* prev;
    state* next;
    symbol* sym;
    token tok;
    std::vector< reducefrom* > rfrom; // links to reachable final transitions.
    std::vector< reducefrom* > rgoto; // link from final transition to nonterminal.
    uintptr_t visited;
    bool conflicts;
};


/*
    Reducefroms link the last transition in a rule (that transitions to a state
    that accepts the rule) with the transition that shifts the resulting
    nonterminal symbol.  Lookahead sets flow from the state after the
    nonterminal transition to the accept state after the last symbol.
*/

struct reducefrom
{
    reducefrom( rule* drule, transition* nonterminal, transition* finalsymbol );

    rule* drule;
    transition* nonterminal; // transition shifting nonterminal symbol.
    transition* finalsymbol; // final transition before accepting nonterminal.
};


/*
    A reduction action in a state.
*/

struct reduction
{
    explicit reduction( rule* drule );

    rule* drule;
    std::vector< terminal* > lookahead;
};




#endif


