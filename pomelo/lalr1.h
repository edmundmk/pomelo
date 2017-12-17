//
//  lalr1.h
//  pomelo
//
//  Created by Edmund Kapusniak on 16/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#ifndef LALR1_H
#define LALR1_H


#include <memory>
#include <deque>
#include <set>
#include <unordered_map>
#include "syntax.h"


struct closure;
struct closure_deleter;
struct closure_key;
struct location_compare;
struct transition;
struct state;
struct automata;
class makedfa;

typedef std::unique_ptr< closure, closure_deleter > closure_ptr;
typedef std::unique_ptr< state > state_ptr;
typedef std::shared_ptr< automata > automata_ptr;
typedef std::shared_ptr< makedfa > makedfa_ptr;



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

struct closure_key
{
    explicit closure_key( closure* p );
    closure* p;
};

bool operator == ( const closure_key& a, const closure_key& b );
bool operator != ( const closure_key& a, const closure_key& b );

template <> struct std::hash< closure_key >
{
    size_t operator () ( const closure_key& a ) const;
};



/*
    We sort locations by the next symbol in the grammar, then by index.
*/

class location_compare
{
public:

    explicit location_compare( syntax* syntax );
    bool operator () ( size_t a, size_t b ) const;

private:

    std::less< symbol* > _less;
    syntax* _syntax;

};




/*
    A discrete finite automata.
*/

struct transition
{
    symbol*     symbol;
    state*      state;
};

struct state
{
    closure_ptr closure;
    std::vector< transition > transitions;
};

struct automata
{
    std::vector< state_ptr > states;
};



/*
    Build a LALR(1) DFA from the rules in the grammar.
*/

class makedfa
{
public:

    makedfa( errors_ptr errors, automata_ptr automata );
    ~makedfa();

    void construct( syntax* syntax );


private:

    void    add_location( syntax* syntax, size_t locindex );
    void    add_transitions( syntax* syntax, state* pstate );
    void    alloc_scratch( size_t capacity );
    state*  close_state();

    errors_ptr _errors;
    automata_ptr _automata;
    std::set< size_t, location_compare > _locations;
    std::unordered_map< closure_key, state* > _states;
    std::deque< state* > _pending;
    size_t _scratch_capacity;
    closure_ptr _scratch_closure;

};





#endif


