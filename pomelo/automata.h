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


struct symbol;
struct closure;
struct closure_deleter;
struct transition;
struct state;
struct automata;

typedef std::unique_ptr< closure, closure_deleter > closure_ptr;
typedef std::unique_ptr< state > state_ptr;
typedef std::shared_ptr< automata > automata_ptr;




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







#endif




