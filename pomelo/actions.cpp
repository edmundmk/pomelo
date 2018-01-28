//
//  actions.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include <assert.h>
#include "actions.h"



actions::actions( errors_ptr errors, automata_ptr automata )
    :   _errors( errors )
    ,   _automata( automata )
    ,   _calculated_distances( true )
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
    
    // Report conflict.
    for ( const auto& state : _automata->states )
    {
        if ( state->has_conflict )
        {
            report_conflicts( state.get() );
        }
    }
}


void actions::traverse_start( state* s, int distance )
{
    if ( s->start_distance <= distance )
    {
        return;
    }
    
    s->start_distance = distance;
    distance += 1;
    for ( transition* transition : s->next )
    {
        traverse_start( transition->next, distance );
    }
}

void actions::left_context( state* s, std::vector< context_production >* out_lcontext )
{
    // Find transition back to any state with a lower distance.
    while ( s->start_distance )
    {
        for ( transition* transition : s->prev )
        {
            if ( transition->prev->start_distance < s->start_distance )
            {
                s = transition->prev;
                out_lcontext->at( s->start_distance ) = { transition->symbol };
                break;
            }
        }
    }
}


void actions::traverse_accept( state* s, int distance )
{
    if ( s->accept_distance <= distance )
    {
        return;
    }
    
    s->accept_distance = distance;
    distance += 1;
    for ( transition* transition : s->prev )
    {
        traverse_accept( transition->prev, distance );
        for ( const auto& rfrom : transition->rfrom )
        {
            traverse_accept( rfrom->finalsymbol->next, distance );
        }
    }
}

void actions::right_context( state* s, std::vector< context_production >* out_rcontext )
{
    // Find transition forward to any state with lower distance, or back
    // from the final symbol of a reduction to a state with lower distance.
    while ( s->accept_distance )
    {
        for ( transition* transition : s->next )
        {
            if ( transition->next->accept_distance < s->accept_distance )
            {
                s = transition->next;
                out_rcontext->push_back( { transition->symbol } );
                goto next_step;
            }
        }

        for ( transition* transition : s->prev )
        {
            for ( reducefrom* rgoto : transition->rgoto )
            {
                if ( rgoto->nonterminal->next->accept_distance < s->accept_distance )
                {
                    s = rgoto->nonterminal->next;
                    reduce_context( rgoto->rule, out_rcontext );
                    goto next_step;
                }
            }
        }

        next_step: ;
    }
}

void actions::reduce_context( rule* rule, context* out_context )
{
    size_t reduce_count = rule->locount - 1;
    context_production prod = { rule->nonterminal };
    prod.children.assign( out_context->end() - reduce_count, out_context->end() );
    out_context->push_back( prod );
}

state* actions::emulate_reduce( state* s, reduction* reduce, context* out_context )
{
    reduce_context( reduce->rule, out_context );
    if ( reduce->rfrom )
    {
        // Follow the link to find the next state.
        return reduce->rfrom->nonterminal->next;
    }
    else
    {
        // Reduced an epsilon, next state is shifting the nonterminal.
        assert( reduce->rule->locount == 1 );
        for ( transition* transition : s->next )
        {
            if ( transition->symbol == reduce->rule->nonterminal )
            {
                return  transition->next;
            }
        }
    }
    
    assert( ! "should not happen" );
}

state* actions::emulate( state* s, terminal* token, context* out_context )
{
    // Emulate parser actions until we can actually shift token.
    while ( true )
    {
        const auto& action = s->actions.at( token->value );
        assert( action.kind != ACTION_ERROR );
        
        switch ( action.kind )
        {
        case ACTION_ERROR:
        {
            assert( ! "should not happen" );
            break;
        }
        
        case ACTION_SHIFT:
        {
            out_context->push_back( { token } );
            return action.shift->next;
        }
        
        case ACTION_REDUCE:
        {
            s = emulate_reduce( s, action.reduce, out_context );
            break;
        }
        
        case ACTION_CONFLICT:
        {
            if ( action.conflict->shift )
            {
                out_context->push_back( { token } );
                return action.conflict->shift->next;
            }
            else
            {
                reduction* reduce = action.conflict->reduce.front();
                s = emulate_reduce( s, reduce, out_context );
            }
            break;
        }
        }
    }
}

std::string actions::print_context( const context& context )
{
    return "";
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
                int old_prec = action->reduce->rule->precedence->precedence;
                int new_prec = reduction->rule->precedence->precedence;
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
    for ( transition* transition : s->next )
    {
        if ( ! transition->symbol->is_terminal )
        {
            continue;
        }
    
        action* action = &s->actions.at( transition->symbol->value );
        
        switch ( action->kind )
        {
            case ACTION_ERROR:
            {
                // Set reduction.
                action->kind = ACTION_SHIFT;
                action->shift = transition;
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
                terminal* shift_symbol = (terminal*)transition->symbol;
                int reduce_prec = action->reduce->rule->precedence->precedence;
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
                    action->shift = transition;
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
                    conflict->shift = transition;
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
                action->conflict->shift = transition;
                break;
            }
        }
    }
}


void actions::report_conflicts( state* s )
{
    // Detailed reporting of conflicts requires pathfinding through the graph.
    if ( ! _calculated_distances )
    {
        _calculated_distances = true;
    
        // Traverse DFA from start state to work out distance from the start.
        traverse_start( _automata->start, 0 );
        
        // Traverse DFA from accept state (and following rfrom links) to work
        // out distance to the accept state.
        traverse_accept( _automata->accept, 0 );
    }
    
    // Group conflicts by whether or not they shift, and by reduction sets.
    for ( size_t i = 0; i < s->actions.size(); )
    {
        action* action = &s->actions.at( i++ );
        if ( action->kind == ACTION_CONFLICT && ! action->conflict->reported )
        {
            std::vector< conflict* > conflicts;
            conflicts.push_back( action->conflict );
            action->conflict->reported = true;
            
            for ( size_t j = i; j < s->actions.size(); ++j )
            {
                ::action* similar = &s->actions.at( i++ );
                if ( similar_conflict( action->conflict, similar->conflict ) )
                {
                    conflicts.push_back( similar->conflict );
                    similar->conflict->reported = true;
                }
            }
            
            report_conflict( s, conflicts );
        }
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
    std::string error = "parsing conflict";

    // Find left context to state.
    context lcontext;
    lcontext.resize( s->start_distance );
    left_context( s, &lcontext );
    
    terminal* next_token = nullptr;
    if ( conflicts.front()->shift )
    {
        // Find example shift with shortest context.
        state* next_state = nullptr;
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
    
        // Calculate example context.
        context scontext = lcontext;
        scontext.push_back( { next_token } );
        right_context( next_state, &scontext );
    
        // Report shift.
        error += "\n    shift";
        for ( conflict* conflict : conflicts )
        {
            assert( conflict->shift );
            error += " ";
            error += source->text( conflict->terminal->name );
        }
        error += " ->\n        ";
        error += print_context( scontext );
    }
    
    // Emulate reductions using the same token, or a random one.
    next_token = next_token ? next_token : conflicts.front()->terminal;

    // Find example for each reduction.  Reductions should be identical.
    for ( reduction* reduce : conflicts.front()->reduce )
    {
        
        // Emulate continuing the parse by reducing then with the terminal.
        context rcontext = lcontext;
        state* next_state = emulate_reduce( s, reduce, &rcontext );
        next_state = emulate( next_state, next_token, &rcontext );
        right_context( next_state, &rcontext );
        
        // Report reduction.
        error += "\n    reduce ";
        error += source->text( reduce->rule->nonterminal->name );
        error += " after";
        for ( conflict* conflict : conflicts )
        {
            const auto& r = conflict->reduce;
            if ( std::find( r.begin(), r.end(), reduce ) != r.end() )
            {
                error += " ";
                error += source->text( conflict->terminal->name );
            }
        }
        error += " ->\n        ";
        error += print_context( rcontext );
    }

    // Actually print conflict.  Make a guess at a related source location.
    srcloc sloc = conflicts.front()->reduce.front()->rule->nonterminal->name.sloc;
    _errors->warning( sloc, "%s", error.c_str() );
}








