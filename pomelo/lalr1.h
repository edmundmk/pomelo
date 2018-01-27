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
#include "automata.h"


struct closure_key;
struct location_compare;
class lalr1;

typedef std::shared_ptr< lalr1 > lalr1_ptr;



/*
    Closure keys used to look up already-constructed states.
*/

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

    syntax* _syntax;

};


/*
    We sort symbols by index.
*/

class terminal_compare
{
public:

    bool operator () ( terminal* a, terminal* b ) const;

};




/*
    Build a LALR(1) DFA from the rules in the grammar.
*/

class lalr1
{
public:

    explicit lalr1( errors_ptr errors, syntax_ptr syntax );
    ~lalr1();

    automata_ptr construct();


private:

    bool    erasable_rule( rule* rule );
    void    add_location( size_t locindex );
    void    add_transitions( state* pstate );
    void    alloc_scratch( size_t capacity );
    state*  close_state();
    void    add_reducefroms( transition* nonterm );
    void    reduce_lookahead( state* state, rule* rule );
    void    follow_lookahead( transition* transition );
    void    direct_lookahead( state* state );
    

    errors_ptr _errors;
    automata_ptr _automata;
    std::set< size_t, location_compare > _locations;
    std::unordered_map< closure_key, state* > _states;
    std::deque< state* > _pending;
    size_t _scratch_capacity;
    closure_ptr _scratch_closure;
    std::set< terminal*, terminal_compare > _lookahead;

};





#endif


