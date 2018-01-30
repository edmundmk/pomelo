//
//  actions.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include <assert.h>
#include "actions.h"
#include "search.h"



actions::actions( errors_ptr errors, automata_ptr automata )
    :   _errors( errors )
    ,   _automata( automata )
{
}

actions::actions()
{
}



void actions::analyze()
{
    // Work out actions for each state.
    for ( const auto& state : _automata->states )
    {
        build_actions( state.get() );
    }
    
    // Traverse automata using actions and check if any rules are unreachable.
    
    
}

void actions::report_conflicts()
{
    // Report conflict.
    for ( const auto& state : _automata->states )
    {
        if ( state->has_conflict )
        {
            report_conflicts( state.get() );
        }
    }
}

void actions::print()
{
    source_ptr source = _automata->syntax->source;
    for ( const auto& state : _automata->states )
    {
        printf( "%p\n", state.get() );
        for ( const auto& entry : _automata->syntax->terminals )
        {
            terminal* term = entry.second.get();
            const action& action = state->actions.at( term->value );
            if ( action.kind == ACTION_ERROR )
            {
                continue;
            }
            
            printf( "    %s -> ", source->text( term->name ) );
            switch ( action.kind )
            {
            case ACTION_ERROR:
                break;
                
            case ACTION_SHIFT:
                printf( "SHIFT %p\n", action.shift->next );
                break;
            
            case ACTION_REDUCE:
                printf( "REDUCE %s\n", source->text( action.reduce->rule->nonterminal->name ) );
                break;
            
            case ACTION_CONFLICT:
                printf( "CONFLICT\n" );
                if ( action.conflict->shift )
                {
                    printf( "        SHIFT %p\n", action.conflict->shift->next );
                }
                for ( reduction* reduction : action.conflict->reduce )
                {
                    printf( "        REDUCE %s\n", source->text( reduction->rule->nonterminal->name ) );
                }
                break;
            }
        }
        for ( transition* trans : state->next )
        {
            if ( trans->symbol->is_terminal )
                continue;
            
            printf( "    ** %s -> %p\n", source->text( trans->symbol->name ), trans->next );
        }
    }
}


void actions::build_actions( state* s )
{
    // All actions in the state start off as errors.
    size_t terminals_count = _automata->syntax->terminals.size();
    s->actions.assign( terminals_count, { ACTION_ERROR } );
    
    // Go through all reductions and check reduce/reduce conflicts.
    for ( reduction* reduction : s->reductions )
    {
        for ( terminal* lookahead : reduction->lookahead )
        {
            action* action = &s->actions.at( lookahead->value );
            
            switch ( action->kind )
            {
            case ACTION_ERROR:
            {
                // Set reduction.
                action->kind = ACTION_REDUCE;
                action->reduce = reduction;
                break;
            }

            case ACTION_SHIFT:
            {
                assert( ! "should never happen" );
                break;
            }
            
            case ACTION_REDUCE:
            {
                // Attempt to resolve with precedence.
                int old_prec = rule_precedence( action->reduce->rule );
                int new_prec = rule_precedence( reduction->rule );
                if ( old_prec != new_prec && old_prec != -1 && new_prec != -1 )
                {
                    // This reduce/reduce conflict is resolved by precedence.
                    if ( new_prec > old_prec )
                    {
                        action->reduce = reduction;
                    }
                }
                else
                {
                    // This is an actual conflict.
                    conflict_ptr conflict = std::make_unique< ::conflict >( lookahead );
                    conflict->reduce.push_back( action->reduce );
                    conflict->reduce.push_back( reduction );
                    action->kind = ACTION_CONFLICT;
                    action->conflict = conflict.get();
                    _automata->conflicts.push_back( std::move( conflict ) );
                    s->has_conflict = true;
                }
                break;
            }
            
            case ACTION_CONFLICT:
            {
                // Add reduction to the conflict.
                action->conflict->reduce.push_back( reduction );
                break;
            }
            }
        }
    }
    
    // Go through all transitions of terminals and check shift/reduce conflicts.
    for ( transition* trans : s->next )
    {
        if ( ! trans->symbol->is_terminal )
        {
            continue;
        }
    
        action* action = &s->actions.at( trans->symbol->value );
        
        switch ( action->kind )
        {
            case ACTION_ERROR:
            {
                // Set reduction.
                action->kind = ACTION_SHIFT;
                action->shift = trans;
                break;
            }

            case ACTION_SHIFT:
            {
                assert( ! "should never happen" );
                break;
            }
            
            case ACTION_REDUCE:
            {
                // Attempt to resolve with precedence.
                terminal* shift_symbol = (terminal*)trans->symbol;
                int reduce_prec = rule_precedence( action->reduce->rule );
                int shift_prec = shift_symbol->precedence;
                
                enum { UNKNOWN, SHIFT, REDUCE, CONFLICT } result = UNKNOWN;
                if ( reduce_prec == -1 || shift_prec == -1 )
                {
                    // Precedence is missing for either shift or reduce.
                    result = CONFLICT;
                }
                else if ( shift_prec > reduce_prec )
                {
                    // Shift precedence is higher.
                    result = SHIFT;
                }
                else if ( reduce_prec > shift_prec )
                {
                    // Reduce precedence is higher.
                    result = REDUCE;
                }
                else if ( shift_symbol->associativity == ASSOC_RIGHT )
                {
                    // Right-associative, resolve in favour of shift.
                    assert( shift_prec == reduce_prec );
                    result = SHIFT;
                }
                else if ( shift_symbol->associativity == ASSOC_LEFT )
                {
                    // Left-associative, resolve in favour of reduce.
                    assert( shift_prec == reduce_prec );
                    result = REDUCE;
                }
                else
                {
                    // Nonassociative with equal precedence, conflict.
                    result = CONFLICT;
                }
                
                switch ( result )
                {
                case UNKNOWN:
                {
                    assert( ! "should never happen" );
                    break;
                }
                
                case SHIFT:
                {
                    action->kind = ACTION_SHIFT;
                    action->shift = trans;
                    break;
                }
                
                case REDUCE:
                {
                    // Reduction is already present in the action.
                    break;
                }
                
                case CONFLICT:
                {
                    conflict_ptr conflict = std::make_unique< ::conflict >( shift_symbol );
                    conflict->shift = trans;
                    conflict->reduce.push_back( action->reduce );
                    action->kind = ACTION_CONFLICT;
                    action->conflict = conflict.get();
                    _automata->conflicts.push_back( std::move( conflict ) );
                    s->has_conflict = true;
                    break;
                }
                }
            }
            
            case ACTION_CONFLICT:
            {
                // Add shift to the conflict.
                action->conflict->shift = trans;
                break;
            }
        }
    }
}

int actions::rule_precedence( rule* r )
{
    return r->precedence ? r->precedence->precedence : -1;
}



void actions::report_conflicts( state* s )
{
    // Detailed reporting of conflicts requires pathfinding through the graph.
    _automata->ensure_distances();
    
    // Group conflicts by whether or not they shift, and by reduction sets.
    for ( size_t i = 0; i < s->actions.size(); )
    {
        action* action = &s->actions.at( i++ );
        if ( action->kind != ACTION_CONFLICT || action->conflict->reported )
        {
            continue;
        }
        
        std::vector< conflict* > conflicts;
        conflicts.push_back( action->conflict );
        action->conflict->reported = true;
        
        for ( size_t j = i; j < s->actions.size(); ++j )
        {
            ::action* similar = &s->actions.at( i++ );
            if ( similar->kind != ACTION_CONFLICT || action->conflict->reported )
            {
                continue;
            }
            
            if ( similar_conflict( action->conflict, similar->conflict ) )
            {
                conflicts.push_back( similar->conflict );
                similar->conflict->reported = true;
            }
        }
            
        report_conflict( s, conflicts );
    }
}


bool actions::similar_conflict( conflict* a, conflict* b )
{
    // Conflicts are only similar if they have a shift (or not have a shift).
    if ( ( a->shift != nullptr ) != ( b->shift != nullptr ) )
        return false;
    
    // Similar conflicts should have the same number of reductions.
    if ( a->reduce.size() != b->reduce.size() )
        return false;
    
    // Reductions should have been added to the conflicts in the same order.
    for ( size_t i = 0; i < a->reduce.size(); ++i )
    {
        if ( a->reduce.at( i ) != b->reduce.at( i ) )
            return false;
    }
    
    return true;
}




void actions::report_conflict( state* s, const std::vector< conflict* >& conflicts )
{
    source_ptr source = _automata->syntax->source;

    // Tokens involved in conflict.
    std::string report = "parsing conflict on";
    for ( conflict* conflict : conflicts )
    {
        report += " ";
        report += source->text( conflict->terminal->name );
    }

    // Work out which token to use in examples.
    state* next_state = nullptr;
    terminal* next_token = nullptr;
    if ( conflicts.front()->shift )
    {
        for ( conflict* conflict : conflicts )
        {
            // Emulate parse with this potential conflict token.
            assert( conflict->shift );
            state* test_state = conflict->shift->next;
            if ( ! next_state || test_state->accept_distance < next_state->accept_distance )
            {
                next_state = test_state;
                next_token = conflict->terminal;
            }
        }
    }
    else
    {
        next_token = conflicts.front()->terminal;
    }
    
    // Find left context which successfully supports all conflicting actions.
    parse_search_ptr sp;
    std::vector< std::pair< rule*, parse_search_ptr > > reducep;
    
    left_search_ptr lsearch = std::make_shared< left_search >( _automata, s );
    while ( left_context_ptr lcontext = lsearch->generate() )
    {
        bool success = true;
    
        if ( conflicts.front()->shift )
        {
            assert( next_state );
            sp = std::make_shared< parse_search >( _automata, lcontext );
            success = success && sp->shift( next_state, next_token );
        }
        
        reducep.clear();
        for ( reduction* reduce : conflicts.front()->reduce )
        {
            auto rp = std::make_shared< parse_search >( _automata, lcontext );
            success = success && rp->reduce( reduce->rule, next_token );
            reducep.emplace_back( reduce->rule, rp );
        }
    
        if ( success )
        {
            break;
        }
    }
    
    // Perform right-context searches and report results.
    if ( sp )
    {
        sp->search();
        report += "\n    shift\n       ";
        sp->print( &report );
    }
    for ( const auto& reduce : reducep )
    {
        reduce.second->search();
        report += "\n    reduce ";
        report += source->text( reduce.first->nonterminal->name );
        report += "\n       ";
        reduce.second->print( &report );
    }

    // Actually print conflict.  Make a guess at a related source location.
    srcloc sloc = conflicts.front()->reduce.front()->rule->nonterminal->name.sloc;
    _errors->warning( sloc, "%s", report.c_str() );
}








