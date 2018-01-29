//
//  actions.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include <assert.h>
#include "actions.h"


bool context::heuristic::operator () ( const shead& a, const shead& b ) const
{
    int acost = a.distance + a.state->accept_distance;
    int bcost = b.distance + b.state->accept_distance;
    return acost > bcost;
}



context::context( automata_ptr automata, state* left_context )
    :   _automata( automata )
    ,   _left( nullptr, left_context, 0 )
    ,   _tree( nullptr, nullptr, 0 )
{
    // Build stack backwards by following transitions back with lower
    // start_distances until we get to the start state.
    state* s = left_context;
    value* back = nullptr;
    while ( s->start_distance )
    {
        for ( transition* transition : s->prev )
        {
            if ( transition->prev->start_distance < s->start_distance )
            {
                s = transition->prev;
                
                value_ptr v = std::make_unique< value >( nullptr, s, transition->symbol );
                if ( back )
                {
                    back->prev = v.get();
                }
                else
                {
                    _left.stack = v.get();
                }
            
                back = v.get();
                _values.push_back( std::move( v ) );
                break;
            }
        }
    }
}

context::~context()
{
}


void context::shift( state* next, terminal* term )
{
    _open.push( shift( _left, next, term ) );
}

context::shead context::shift( const shead& head, state* next, symbol* sym )
{
//    printf( "-->> %s\n", _automata->syntax->source->text( sym->name ) );
    value_ptr v = std::make_unique< value >( head.stack, next, sym );
    shead move( v.get(), next, head.distance + 1 );
    _values.push_back( std::move( v ) );
    return move;
}


void context::reduce( rule* rule, terminal* term )
{
    parse( reduce( _left, rule ), term );
}

context::shead context::reduce( const shead& head, rule* rule )
{
/*    printf( ">>>> %p %d\n", head.state, head.distance );
    for ( value* v = head.stack; v; v = v->prev )
    {
        printf( "    %p %s", v->state, _automata->syntax->source->text( v->symbol->name ) );
        if ( v->ctail )
        {
            std::string result;
            print( v->ctail, v->chead, &result );
            printf( " [ %s ]\n", result.c_str() );
        }
        else
        {
            printf( "\n" );
        }
    }
*/
    // Pop stack.
    size_t reduce_count = rule->locount - 1;
    state* valstate = head.state;
    value* valprev = head.stack;
    value* chead = nullptr;
    value* ctail = nullptr;
    if ( reduce_count )
    {
        ctail = head.stack;
        for ( size_t i = 0; i < reduce_count; ++i )
        {
            chead = valprev;
            valstate = valprev->state;
            valprev = valprev->prev;
        }
    }
    
    // Find state after shifting reduction.
    state* next_state = nullptr;
    for ( transition* transition : valstate->next )
    {
        if ( transition->symbol == rule->nonterminal )
        {
            next_state = transition->next;
            break;
        }
    }
    assert( next_state );
    
    // Create new value and new head.
    value_ptr v = std::make_unique< value >( valprev, valstate, rule->nonterminal );
    v->chead = chead;
    v->ctail = ctail;
    shead move( v.get(), next_state, head.distance + 1 );
    _values.push_back( std::move( v ) );

/*
    printf( "<<<< %p %d\n", move.state, move.distance );
    for ( value* v = move.stack; v; v = v->prev )
    {
        printf( "    %p %s", v->state, _automata->syntax->source->text( v->symbol->name ) );
        if ( v->ctail )
        {
            std::string result;
            print( v->ctail, v->chead, &result );
            printf( " [ %s ]\n", result.c_str() );
        }
        else
        {
            printf( "\n" );
        }
    }
*/
    return move;
}

void context::search()
{
    while ( _open.size() )
    {
        // Get lowest-cost parse state.
        shead head = _open.top();
        _open.pop();
        
        // If the parse state accepts, we are done.
        if ( ! head.state->accept_distance )
        {
            _tree = head;
            return;
        }
        
        // Evaluate all valid tokens from this state.
        for ( const auto& entry : _automata->syntax->terminals )
        {
            terminal* term = entry.second.get();
            parse( head, term );
        }
        
        // Also 'shift' nonterminals, in case that gets us to the accept state
        // quicker.
        for ( transition* transition : head.state->next )
        {
            if ( transition->symbol->is_terminal )
            {
                continue;
            }
            _open.push( shift( head, transition->next, transition->symbol ) );
        }
    }
    
    // This ended up invalid, probably because the left context is insufficient.
    _tree = _left;
}

void context::parse( const shead& head, terminal* term )
{
//    printf( "---> %s\n", _automata->syntax->source->text( term->name ) );

    // Perform a parsing step from head.
    shead move = head;
    while ( true )
    {
        const action& action = move.state->actions.at( term->value );
        switch ( action.kind )
        {
        case ACTION_ERROR:
            // Parse error results from this branch.
            return;
            
        case ACTION_SHIFT:
            // Shift terminal and then return.
            _open.push( shift( move, action.shift->next, term ) );
            return;
            
        case ACTION_REDUCE:
            // Reduce terminal and continue until we can shift.
            move = reduce( move, action.reduce->rule );
            continue;
        
        case ACTION_CONFLICT:
            // Perform all possible actions.
            if ( action.conflict->shift )
            {
                _open.push( shift( move, action.conflict->shift->next, term ) );
            }
            for ( reduction* reduction : action.conflict->reduce )
            {
                parse( reduce( move, reduction->rule ), term );
            }
            return;
        }
    }
}

void context::print( std::string* out_print )
{
    if ( _tree.stack && _tree.stack->prev && _tree.stack->prev->ctail )
    {
        value* v = _tree.stack->prev;
        print( v->ctail, v->chead, out_print );
    }
    else
    {
        print( _tree.stack, nullptr, out_print );
    }
}

void context::print( const value* tail, const value* head, std::string* out_print )
{
    // Get values in order.
    std::vector< const value* > values;
    for ( const value* child = tail; child; child = child->prev )
    {
        values.push_back( child );
        if ( child == head )
        {
            break;
        }
    }
    std::reverse( values.begin(), values.end() );

    // Print values.
    source_ptr source = _automata->syntax->source;
    for ( const value* v : values )
    {
        if ( v->ctail )
        {
            out_print->append( " [" );
            print( v->ctail, v->chead, out_print );
            out_print->append( " ]" );
        }
        else
        {
            out_print->append( " " );
            out_print->append( source->text( v->symbol->name ) );
            if ( v == _left.stack )
            {
                out_print->append( " ·" );
            }
        }
    }
}





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
    
    // Traverse automata using actions and check if any rules are 
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
        for ( transition* transition : state->next )
        {
            if ( transition->symbol->is_terminal )
                continue;
            
            printf( "    ** %s -> %p\n", source->text( transition->symbol->name ), transition->next );
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

    // Find example shift with shortest context.
    terminal* next_token = nullptr;
    if ( conflicts.front()->shift )
    {
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
        context scontext( _automata, s );
        scontext.shift( next_state, next_token );
        scontext.search();
    
        // Report shift.
        report += "\n    shift\n       ";
        scontext.print( &report );
    }
    
    // Emulate reductions using the same token, or a random one.
    next_token = next_token ? next_token : conflicts.front()->terminal;

    // Find example for each reduction.  Reductions should be identical.
    for ( reduction* reduce : conflicts.front()->reduce )
    {
        // Emulate continuing the parse by reducing then with the terminal.
        context rcontext( _automata, s );
        rcontext.reduce( reduce->rule, next_token );
        rcontext.search();
        
        // Report reduction.
        report += "\n    reduce ";
        report += source->text( reduce->rule->nonterminal->name );
        report += "\n       ";
        rcontext.print( &report );
    }

    // Actually print conflict.  Make a guess at a related source location.
    srcloc sloc = conflicts.front()->reduce.front()->rule->nonterminal->name.sloc;
    _errors->warning( sloc, "%s", report.c_str() );
}








