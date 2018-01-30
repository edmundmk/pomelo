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
    Compare terminals so lower values are first.
*/

bool terminal_compare::operator () ( terminal* a, terminal* b ) const
{
    return a->value < b->value;
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
    // Find erasable nonterminals.
    bool changed = true;
    while ( changed )
    {
        changed = false;
        for ( const auto& entry : _automata->syntax->nonterminals )
        {
            nonterminal* nsym = entry.second.get();

            // If it's already erasable there's nothing to do.
            if ( nsym->erasable )
            {
                continue;
            }

            // A nonterminal is erasable if there is at least one rule
            // consisting only of erasable symbols.
            for ( const auto& rule : nsym->rules )
            {
                if ( erasable_rule( rule.get() ) )
                {
                    nsym->erasable = true;
                    changed = true;
                    break;
                }
            }
        }
    }

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

    // Calculate reducefroms, which link final transitions back
    // to the transitions which shift the newly-reduced symbol.
    for ( const auto& transition : _automata->transitions )
    {
        if ( ! transition->symbol->is_terminal )
        {
            add_reducefroms( transition.get() );
        }
    }
    
    // Calculate lookahead sets for reductions.
    for ( const auto& state : _automata->states )
    {
        for ( size_t i = 0; i < state->closure->size; ++i )
        {
            size_t iloc = state->closure->locations[ i ];
            const location& loc = _automata->syntax->locations[ iloc ];
            if ( ! loc.symbol )
            {
                reduce_lookahead( state.get(), loc.rule );
            }
        }
    }
    
    // Return constructed automata.
    return _automata;
}


bool lalr1::erasable_rule( rule* rule )
{
    // A rule is erasable if all symbols in it are erasable.
    for ( size_t i = 0; i < rule->locount - 1; ++i )
    {
        size_t iloc = rule->lostart + i;
        const location& loc = _automata->syntax->locations.at( iloc );
        if ( loc.symbol->is_terminal || !( (nonterminal*)loc.symbol )->erasable )
        {
            return false;
        }
    }
    return true;
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
        
        // If the next symbol is null the following locations all reduce.
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
        transition_ptr trans = std::make_unique< transition >( pstate, nstate, nsym, l.stoken );
        pstate->next.push_back( trans.get() );
        nstate->prev.push_back( trans.get() );
        _automata->transitions.push_back( std::move( trans ) );
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
    size_t size = sizeof( closure ) + sizeof( size_t ) * _scratch_closure->size;
    closure_ptr closure = closure_ptr( (::closure*)malloc( size ) );
    closure->hash = _scratch_closure->hash;
    closure->size = _scratch_closure->size;
    memcpy
    (
        closure->locations,
        _scratch_closure->locations,
        sizeof( size_t ) * _scratch_closure->size
    );
    state_ptr nstate = std::make_unique< state >( std::move( closure ) );
    
    // Add to bookkeeping.
    state* pstate = nstate.get();

    if ( _automata->states.empty() )
    {
        assert( ! _automata->start );
        _automata->start = pstate;
    }

    size_t iloc = pstate->closure->locations[ 0 ];
    const auto& loc = _automata->syntax->locations[ iloc ];
    if ( loc.rule->nonterminal == _automata->syntax->start && ! loc.symbol )
    {
        assert( ! _automata->accept );
        _automata->accept = pstate;
    }

    _automata->states.push_back( std::move( nstate ) );
    _states.emplace( closure_key( pstate->closure.get() ), pstate );
    _pending.push_back( pstate );
    return pstate;
}


void lalr1::add_reducefroms( transition* nonterm )
{
    // This transition is a nonterminal.
    assert( ! nonterm->symbol->is_terminal );
    nonterminal* nsym = (nonterminal*)nonterm->symbol;

    // Follow each rule to find the reduction transition.
    for ( const auto& rule : nsym->rules )
    {
        transition* fsymbol = nullptr;
        state* state = nonterm->prev;
        
        for ( size_t i = 0; i < rule->locount - 1; ++i )
        {
            size_t iloc = rule->lostart + i;
            const location& loc = _automata->syntax->locations[ iloc ];
            
            // Follow transition.
            for ( transition* trans : state->next )
            {
                if ( trans->symbol == loc.symbol )
                {
                    fsymbol = trans;
                    state = trans->next;
                    break;
                }
            }
        }
        
        // Add lookback from the final transition to the nonterminal one.
        if ( fsymbol )
        {
            reducefrom_ptr rfrom = std::make_unique< reducefrom >( rule.get(), nonterm, fsymbol );
            nonterm->rfrom.push_back( rfrom.get() );
            fsymbol->rgoto.push_back( rfrom.get() );
            _automata->reducefrom.push_back( std::move( rfrom ) );
        }
    }
}



void lalr1::reduce_lookahead( state* state, rule* rule )
{
    _automata->visited += 1;
    
    // Find lookahead by finding transitions that shift the reduced symbol.
    if ( rule->locount > 1 )
    {
        // If it's not an epsilon reduction, then we need to follow the links
        // back from the final transitions of the rule.
        for ( transition* trans : state->prev )
        {
            for ( reducefrom* reducefrom : trans->rgoto )
            {
                if ( reducefrom->rule == rule )
                {
                    assert( reducefrom->finalsymbol == trans );
                    follow_lookahead( reducefrom->nonterminal );
                }
            }
        }
    }
    else
    {
        // Otherwise the transition that shifts the reduced nonterminal is one
        // of the next transitions from this state.
        for ( transition* trans : state->next )
        {
            if ( trans->symbol == rule->nonterminal )
            {
                follow_lookahead( trans );
                break;
            }
        }
    }
    
    // Convert to reduction lookahead set.
    reduction_ptr reduction = std::make_unique< ::reduction >( rule );
    for ( const auto& terminal : _lookahead )
    {
        reduction->lookahead.push_back( terminal );
    }
    _lookahead.clear();
    state->reductions.push_back( reduction.get() );
    _automata->reductions.push_back( std::move( reduction ) );
}


void lalr1::follow_lookahead( transition* trans )
{
    // Cycles in these links occur in an ambiguous grammar.
    if ( trans->visited == _automata->visited )
    {
        return;
    }
    
    // We need to follow any other links from here to cover the cases where
    // a final symbol is itself a nonterminal.
    trans->visited = _automata->visited;
    direct_lookahead( trans->next );
    for ( const auto& nestfinal : trans->rgoto )
    {
        assert( nestfinal->finalsymbol == trans );
        follow_lookahead( nestfinal->nonterminal );
    }
}


void lalr1::direct_lookahead( state* state )
{
    // Don't visit the same state twice.
    if ( state->visited == _automata->visited )
    {
        return;
    }

    // Direct lookahead from a state is all terminal symbols on a transition
    // out of a state, plus the direct lookahead from any successor state where
    // the transition symbol is eraseable.
    state->visited = _automata->visited;
    for ( transition* trans : state->next )
    {
        if ( trans->symbol->is_terminal )
        {
            _lookahead.insert( (terminal*)trans->symbol );
        }
        else if ( ( (nonterminal*)trans->symbol )->erasable )
        {
            direct_lookahead( trans->next );
        }
    }
}



