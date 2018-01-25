//
//  automata.hpp
//  pomelo
//
//  Created by Edmund Kapusniak on 17/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#ifndef AUTOMATA_H
#define AUTOMATA_H


#include <memory>
#include <vector>
#include "syntax.h"


struct symbol;
struct closure;
struct closure_deleter;
struct state;
struct automata;
struct transition;
struct lookback;


typedef std::unique_ptr< closure, closure_deleter > closure_ptr;
typedef std::shared_ptr< automata > automata_ptr;
typedef std::unique_ptr< state > state_ptr;
typedef std::unique_ptr< transition > transition_ptr;
typedef std::unique_ptr< lookback > lookback_ptr;




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

    void print();

    syntax_ptr syntax;
    std::vector< state_ptr > states;
};

struct state
{
    closure_ptr closure;
    std::vector< transition_ptr > transitions;
};

struct transition
{
    state* prev;
    state* next;
    symbol* symbol;
    lookback_ptr lookback;
};

struct lookback
{
    std::vector< transition* > lookback;
};




#endif


