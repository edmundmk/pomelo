//
//  lalr1.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 16/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


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
        // Compare symbols.
        return _less( al.symbol, bl.symbol );
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

lalr1::lalr1( errors_ptr errors, automata_ptr automata )
    :   _errors( errors )
    ,   _automata( automata )
    ,   _locations( location_compare( nullptr ) )
    ,   _scratch_capacity( 0 )
{
}

lalr1::~lalr1()
{
}


void lalr1::construct( syntax* syntax )
{
    _locations = std::set< size_t, location_compare >( location_compare( syntax ) );

    // Construct initial state.
    for ( const auto& rule : syntax->start->rules )
    {
        add_location( syntax, rule->lostart );
    }
    close_state();
    
    // While there are pending states, process them.
    while ( _pending.size() )
    {
        state* next = _pending.front();
        _pending.pop_front();
        add_transitions( syntax, next );
    }

    // Now work out LALR(1) follow sets.
    // TODO.
}


void lalr1::add_location( syntax* syntax, size_t locindex )
{
    // Add location.
    if ( ! _locations.insert( locindex ).second )
    {
        // Already in the list.
        return;
    }
    
    // If the next symbol is a nonterminal, add initial states for symbol.
    const location& l = syntax->locations.at( locindex );
    if ( l.symbol && ! l.symbol->is_terminal )
    {
        nonterminal* nsym = (nonterminal*)l.symbol;
        for ( const auto& rule : nsym->rules )
        {
            add_location( syntax, rule->lostart );
        }
    }
}


void lalr1::add_transitions( syntax* syntax, state* pstate )
{
    // Construct each final state.
    size_t i = 0;
    while ( i < pstate->closure->size )
    {
        // Find next symbol.
        size_t iloc = pstate->closure->locations[ i ];
        const location& l = syntax->locations.at( iloc );
        symbol* nsym = l.symbol;
        
        // Find all locations which have this as the next symbol.
        while ( i < pstate->closure->size )
        {
            size_t iloc = pstate->closure->locations[ i ];
            const location& l = syntax->locations.at( iloc );
            
            // Check if this location can shift the same symbol.
            if ( l.symbol != nsym )
            {
                break;
            }
            
            // Add following location to the state.
            if ( l.symbol )
            {
                add_location( syntax, iloc + 1 );
            }
        }
        
        // Close state.
        state* nstate = close_state();
        pstate->transitions.push_back( { nsym, nstate } );
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




