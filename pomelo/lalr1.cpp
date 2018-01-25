//
//  lalr1.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 16/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#include <assert.h>
#include "lalr1.h"



/*
    Wrapper for closure pointers so they can be unordered_map keys.
*/

closure_key::closure_key( closure* p )
    :   p( p )
{
}

bool operator == ( const closure_key& a, const closure_key& b )
{
    return a.p->hash == b.p->hash
        && a.p->size == b.p->size
        && memcmp( a.p->locations, b.p->locations, sizeof( size_t ) * a.p->size ) == 0;
}

bool operator != ( const closure_key& a, const closure_key& b )
{
    return ! operator == ( a, b );
}


size_t std::hash< closure_key >::operator () ( const closure_key& a ) const
{
    return a.p->hash;
}



/*
    Compares location indexes such that they are grouped by next symbol.
*/

location_compare::location_compare( syntax* syntax )
    :   _syntax( syntax )
{
}

bool location_compare::operator () ( size_t a, size_t b ) const
{
    const location& al = _syntax->locations.at( a );
    const location& bl = _syntax->locations.at( b );
    
    if ( al.symbol != bl.symbol )
    {
        // Compare symbols.  null symbols are greater than all others.
        if ( al.symbol == nullptr )
            return false;
        if ( bl.symbol == nullptr )
            return true;
        return al.symbol->value < bl.symbol->value;
    }
    else
    {
        // Compare location indexes.
        return a < b;
    }
}





/*
    Builds a LALR(1) DFA from the grammar.
*/

lalr1::lalr1( errors_ptr errors, syntax_ptr syntax )
    :   _errors( errors )
    ,   _automata( std::make_shared< automata >( syntax ) )
    ,   _locations( location_compare( syntax.get() ) )
    ,   _scratch_capacity( 0 )
{
}

lalr1::~lalr1()
{
}


automata_ptr lalr1::construct()
{
    // Construct initial state.
    for ( const auto& rule : _automata->syntax->start->rules )
    {
        add_location( rule->lostart );
    }
    close_state();
    
    // While there are pending states, process them.
    while ( _pending.size() )
    {
        state* next = _pending.front();
        _pending.pop_front();
        add_transitions( next );
    }

    // Calculate lookback relations, which link reduction transitions back
    // to the transitions which shift the newly-reduced symbol.
    for ( const auto& state : _automata->states )
    {
        for ( const auto& transition : state->transitions )
        {
            if ( ! transition->symbol->is_terminal )
            {
                add_lookback( transition.get() );
            }
        }
    }
    
    // Return constructed automata.
    return _automata;
}


void lalr1::add_location( size_t locindex )
{
    // Add location.
    if ( ! _locations.insert( locindex ).second )
    {
        // Already in the list.
        return;
    }
    
    // If the next symbol is a nonterminal, add initial states for symbol.
    const location& l = _automata->syntax->locations.at( locindex );
    if ( l.symbol && ! l.symbol->is_terminal )
    {
        nonterminal* nsym = (nonterminal*)l.symbol;
        for ( const auto& rule : nsym->rules )
        {
            add_location( rule->lostart );
        }
    }
}


void lalr1::add_transitions( state* pstate )
{
    // Construct each following state.  Locations in the closure are sorted
    // by the next symbol in their rule, so rules are grouped.
    size_t i = 0;
    while ( i < pstate->closure->size )
    {
        // Find next symbol.
        size_t iloc = pstate->closure->locations[ i ];
        const location& l = _automata->syntax->locations.at( iloc );
        symbol* nsym = l.symbol;
        
        // If the next symbol is null or eof, the following locations all reduce.
        if ( ! nsym )
        {
            break;
        }
        
        // Find all other locations which have this as the next symbol.
        while ( i < pstate->closure->size )
        {
            size_t iloc = pstate->closure->locations[ i ];
            const location& l = _automata->syntax->locations.at( iloc );
            
            // Check if this location can shift the same symbol.
            if ( l.symbol != nsym )
            {
                break;
            }
            
            // Add following location to the state.
            if ( l.symbol )
            {
                add_location( iloc + 1 );
            }
            
            ++i;
        }
        
        // Close state.
        state* nstate = close_state();
        transition_ptr trans = std::make_unique< transition >();
        trans->prev = pstate;
        trans->next = nstate;
        trans->symbol = nsym;
        pstate->transitions.push_back( std::move( trans ) );
    }
}


void lalr1::add_lookback( transition* reduce )
{
    // This transition is a nonterminal.
    assert( ! reduce->symbol->is_terminal );
    nonterminal* nsym = (nonterminal*)reduce->symbol;

    // Follow each rule to find the reduction transition.
    for ( const auto& rule : nsym->rules )
    {
        transition* final = nullptr;
        state* state = reduce->prev;
        
        for ( size_t i = 0; i < rule->locount - 1; ++i )
        {
            size_t iloc = rule->lostart + i;
            const location& loc = _automata->syntax->locations[ iloc ];
            
            // Follow transition.
            ::state* prev = state;
            for ( const auto& transition : state->transitions )
            {
                if ( transition->symbol == loc.symbol )
                {
                    final = transition.get();
                    state = transition->next;
                    break;
                }
            }
            assert( state != prev );
        }
        
        // Add lookback from the final transition to the original one.
        if ( final )
        {
            if ( ! final->lookback )
            {
                final->lookback = std::make_unique< lookback >();
            }
            final->lookback->lookback.push_back( reduce );
        }
    }
}


void lalr1::alloc_scratch( size_t capacity )
{
    if ( _scratch_capacity < capacity )
    {
        size_t size = sizeof( closure ) + sizeof( size_t ) * capacity;
        _scratch_closure = closure_ptr( (closure*)malloc( size ) );
    }
}


state* lalr1::close_state()
{
    // Create closure.
    std::hash< size_t > hash;
    alloc_scratch( _locations.size() );
    _scratch_closure->hash = 0;
    _scratch_closure->size = 0;
    for ( size_t iloc : _locations )
    {
        _scratch_closure->hash ^= hash( iloc );
        _scratch_closure->locations[ _scratch_closure->size++ ] = iloc;
    }
    _locations.clear();
    
    // Look up state based on closure.
    auto i = _states.find( closure_key( _scratch_closure.get() ) );
    if ( i != _states.end() )
    {
        return i->second;
    }
    
    // Otherwise make new state.
    state_ptr nstate = std::make_unique< state >();
    size_t size = sizeof( closure ) + sizeof( size_t ) * _scratch_closure->size;
    nstate->closure = closure_ptr( (closure*)malloc( size ) );
    nstate->closure->hash = _scratch_closure->hash;
    nstate->closure->size = _scratch_closure->size;
    memcpy
    (
        nstate->closure->locations,
        _scratch_closure->locations,
        sizeof( size_t ) * _scratch_closure->size
    );
    
    // Add to bookkeeping.
    state* pstate = nstate.get();
    _automata->states.push_back( std::move( nstate ) );
    _states.emplace( closure_key( pstate->closure.get() ), pstate );
    _pending.push_back( pstate );
    return pstate;
}




