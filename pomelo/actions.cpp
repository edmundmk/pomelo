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



actions::actions( errors_ptr errors, automata_ptr automata, bool expected_info )
    :   _errors( errors )
    ,   _automata( automata )
    ,   _expected_info( expected_info )
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
    _automata->visited += 1;
    traverse_rules( _automata->start );
    
    // Report any unreachable rules and assign rule indices.
    int index = 0;
    for ( const auto& rule : _automata->syntax->rules )
    {
        if ( rule->reachable )
        {
            rule->index = index++;
        }
        else
        {
            srcloc sloc = rule_location( rule.get() );
            const char* prod = _automata->syntax->source->text( rule->nterm->name );
            _errors->error( sloc, "this rule for production %s cannot be reduced", prod );
        }
    }
    
    _automata->rule_count = index;
    
    // Assign state indices.
    index = 0;
    for ( const auto& state : _automata->states )
    {
        if ( state->reachable )
        {
            state->index = index++;
        }
    }
    
    _automata->state_count = index;
}


action_table_ptr actions::build_action_table()
{
    action_table_ptr table = std::make_shared< action_table >();
    
    // Table dimensions.
    table->rule_count = _automata->rule_count;
    table->conflict_count = -1;
    table->token_count = (int)_automata->syntax->terminals.size();
    table->state_count = _automata->state_count;
    
    // Collect rules.
    for ( const auto& rule : _automata->syntax->rules )
    {
        if ( ! rule->reachable )
            continue;
        
        assert( rule->index == (int)table->rules.size() );
        table->rules.push_back( rule.get() );
    }
    
    // Construct table state by state.
    for ( const auto& state : _automata->states )
    {
        if ( ! state->reachable )
            continue;
        
        size_t base = table->actions.size();
        assert( state->index == (int)base / table->token_count );
        table->actions.insert
        (
            table->actions.end(),
            table->token_count,
            -1
        );
        
        assert( (int)state->actions.size() == table->token_count );
        for ( size_t i = 0; i < state->actions.size(); ++i )
        {
            const action& action = state->actions.at( i );
            int actval = -1;

            switch ( action.kind )
            {
            case ACTION_ERROR:
                actval = -1;
                break;
            
            case ACTION_SHIFT:
                if ( action.shift->next != _automata->accept )
                {
                    assert( action.shift->next->index != -1 );
                    actval = action.shift->next->index;
                }
                else
                {
                    actval = -2;
                }
                break;
            
            case ACTION_REDUCE:
                assert( action.reduce->drule->index != -1 );
                actval = table->state_count + action.reduce->drule->index;
                break;
            
            case ACTION_CONFLICT:
                actval = conflict_actval( table.get(), action.cflict );
                break;
            }
            
            table->actions[ base + i ] = actval;
        }
    }
    
    // Work out remaining action numbers.
    table->conflict_count = (int)table->conflicts.size();
    table->error_action = table->state_count + table->rule_count + table->conflict_count;
    table->accept_action = table->error_action + 1;
    
    // Reset special actions now we know what to call them.
    for ( size_t i = 0; i < table->actions.size(); ++i )
    {
        int action = table->actions.at( i );
        if ( action == -1 )
        {
            table->actions[ i ] = table->error_action;
        }
        else if ( action == -2 )
        {
            table->actions[ i ] = table->accept_action;
        }
    }
    
    return table;
}



goto_table_ptr actions::build_goto_table()
{
    goto_table_ptr table = std::make_shared< goto_table >();
    
    // Table dimensions.
    table->token_count = (int)_automata->syntax->terminals.size();
    table->nterm_count = (int)_automata->syntax->nonterminals.size();
    table->state_count = _automata->state_count;
    
    // Construct table state by state.
    for ( const auto& state : _automata->states )
    {
        if ( ! state->reachable )
            continue;
        
        size_t base = table->gotos.size();
        assert( state->index == (int)base / table->nterm_count );
        table->gotos.insert
        (
            table->gotos.end(),
            table->nterm_count,
            table->state_count
        );
        
        for ( transition* trans : state->next )
        {
            if ( trans->sym->is_terminal )
                continue;
            
            nonterminal* nterm = (nonterminal*)trans->sym;
            assert( nterm->value >= table->token_count );
            assert( nterm->value < table->token_count + table->nterm_count );
            int value = nterm->value - table->token_count;

            assert( trans->next->index != -1 );
            table->gotos[ base + value ] = trans->next->index;
        }
    }
    
    return table;
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
                printf( "REDUCE %s\n", source->text( action.reduce->drule->nterm->name ) );
                break;
            
            case ACTION_CONFLICT:
                printf( "CONFLICT\n" );
                if ( action.cflict->shift )
                {
                    printf( "        SHIFT %p\n", action.cflict->shift->next );
                }
                for ( reduction* reduction : action.cflict->reduce )
                {
                    printf( "        REDUCE %s\n", source->text( reduction->drule->nterm->name ) );
                }
                break;
            }
        }
        for ( transition* trans : state->next )
        {
            if ( trans->sym->is_terminal )
                continue;
            
            printf( "    ** %s -> %p\n", source->text( trans->sym->name ), trans->next );
        }
    }
}


void actions::build_actions( state* s )
{
    source_ptr source = _automata->syntax->source;

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
                int old_prec = rule_precedence( action->reduce->drule );
                int new_prec = rule_precedence( reduction->drule );
                if ( old_prec != new_prec && old_prec != -1 && new_prec != -1 )
                {
                    // This reduce/reduce conflict is resolved by precedence.
                    ::reduction* winner;
                    if ( new_prec > old_prec )
                    {
                        winner = reduction;
                    }
                    else
                    {
                        winner = action->reduce;
                    }
                    
                    if ( _expected_info )
                    {
                        _errors->info
                        (
                            rule_location( winner->drule ),
                            "conflict on %s reduce %s/reduce %s resolved in favour of reduce %s",
                            source->text( lookahead->name ),
                            source->text( action->reduce->drule->nterm->name ),
                            source->text( reduction->drule->nterm->name ),
                            source->text( winner->drule->nterm->name )
                        );
                    }
                    
                    action->reduce = winner;
                }
                else
                {
                    // This is an actual conflict.
                    conflict_ptr conflict = std::make_unique< ::conflict >( lookahead );
                    conflict->reduce.push_back( action->reduce );
                    conflict->reduce.push_back( reduction );
                    action->kind = ACTION_CONFLICT;
                    action->cflict = conflict.get();
                    _automata->conflicts.push_back( std::move( conflict ) );
                    s->has_conflict = true;
                }
                break;
            }
            
            case ACTION_CONFLICT:
            {
                // Add reduction to the conflict.
                action->cflict->reduce.push_back( reduction );
                break;
            }
            }
        }
    }
    
    // Go through all transitions of terminals and check shift/reduce conflicts.
    for ( transition* trans : s->next )
    {
        if ( ! trans->sym->is_terminal )
        {
            continue;
        }
    
        action* action = &s->actions.at( trans->sym->value );
        
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
                terminal* shift_symbol = (terminal*)trans->sym;
                int reduce_prec = rule_precedence( action->reduce->drule );
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
                    if ( _expected_info )
                    {
                        _errors->info
                        (
                            trans->tok.sloc,
                            "conflict on %s shift/reduce %s resolved in favour of shift",
                            source->text( shift_symbol->name ),
                            source->text( action->reduce->drule->nterm->name )
                        );
                    }
                    action->kind = ACTION_SHIFT;
                    action->shift = trans;
                    break;
                }
                
                case REDUCE:
                {
                    // Reduction is already present in the action.
                    if ( _expected_info )
                    {
                        _errors->info
                        (
                            rule_location( action->reduce->drule ),
                            "conflict on %s shift/reduce %s resolved in favour of reduce %s",
                            source->text( shift_symbol->name ),
                            source->text( action->reduce->drule->nterm->name ),
                            source->text( action->reduce->drule->nterm->name )
                        );
                    }
                    break;
                }
                
                case CONFLICT:
                {
                    conflict_ptr conflict = std::make_unique< ::conflict >( shift_symbol );
                    conflict->shift = trans;
                    conflict->reduce.push_back( action->reduce );
                    action->kind = ACTION_CONFLICT;
                    action->cflict = conflict.get();
                    _automata->conflicts.push_back( std::move( conflict ) );
                    s->has_conflict = true;
                    break;
                }
                }
                break;
            }
            
            case ACTION_CONFLICT:
            {
                // Add shift to the conflict.  TODO: check precedence against
                // all reductions if they are all either SHIFT or REDUCE.
                action->cflict->shift = trans;
                break;
            }
        }
    }
}

int actions::rule_precedence( rule* r )
{
    return r->precedence ? r->precedence->precedence : -1;
}

srcloc actions::rule_location( rule* r )
{
    size_t iloc = r->lostart;
    const location& loc = _automata->syntax->locations[ iloc ];
    return loc.stoken.sloc;
}




void actions::traverse_rules( state* s )
{
    // If we've already been here, ignore this state.
    if ( s->visited == _automata->visited )
    {
        return;
    }

    // This state is reachable.
    s->visited = _automata->visited;
    s->reachable = true;
    
    // If this is the accept state, the start rule is reachable.
    if ( s == _automata->accept )
    {
        for ( reduction* reduce : s->reductions )
        {
            reduce->drule->reachable = true;
        }
        return;
    }
    
    // If any of the nonterminal transitions have been visited, then we can
    // now traverse them as both this state and the reduction that branches
    // off here are reachable.
    for ( transition* trans : s->next )
    {
        if ( ! trans->sym->is_terminal && trans->visited == _automata->visited )
        {
            traverse_rules( trans->next );
        }
    }

    // Follow all actions.
    for ( const action& action : s->actions )
    {
        switch ( action.kind )
        {
        case ACTION_ERROR:
            break;
        
        case ACTION_SHIFT:
            traverse_rules( action.shift->next );
            break;
        
        case ACTION_REDUCE:
            traverse_reduce( s, action.reduce );
            break;
        
        case ACTION_CONFLICT:
            if ( action.cflict->shift )
            {
                traverse_rules( action.cflict->shift->next );
            }
            for ( reduction* reduce : action.cflict->reduce )
            {
                traverse_reduce( s, reduce );
            }
            break;
        }
    }
}

void actions::traverse_reduce( state* s, reduction* reduce )
{
    // This rule is reachable.
    reduce->drule->reachable = true;
    
    // If the rule is an epsilon rule, traverse forward from this state.
    if ( reduce->drule->locount <= 1 )
    {
        for ( transition* trans : s->next )
        {
            if ( trans->sym == reduce->drule->nterm )
            {
                trans->visited = _automata->visited;
                assert( trans->prev->visited == _automata->visited );
                traverse_rules( trans->next );
                break;
            }
        }
        return;
    }
    
    // Otherwise, look back along the goto links.
    for ( transition* prev : s->prev )
    {
        for ( reducefrom* rgoto : prev->rgoto )
        {
            if ( rgoto->drule == reduce->drule )
            {
                transition* nterm = rgoto->nonterminal;
                assert( ! nterm->sym->is_terminal );
                
                // This nonterminal transition has been visited.
                nterm->visited = _automata->visited;
            
                // We can traverse into the goto state if the state before it
                // has been visisted, otherwise it will be traversed when and
                // if the state is visted.
                if ( nterm->prev->visited == _automata->visited )
                {
                    traverse_rules( nterm->next );
                }
            }
        }
    }
}



int actions::conflict_actval( action_table* table, conflict* conflict )
{
    // Build conflict.
    int size = 1 + ( conflict->shift ? 1 : 0 ) + (int)conflict->reduce.size();
    int action[ size ];
    
    size_t i = 0;
    action[ i++ ] = size;
    if ( conflict->shift )
    {
        assert( conflict->shift->next->index != -1 );
        action[ i++ ] = conflict->shift->next->index;
    }
    for ( reduction* reduce : conflict->reduce )
    {
        assert( reduce->drule->index != -1 );
        action[ i++ ] = table->state_count + reduce->drule->index;
    }
    
    // Check if it already exists.
    int index = 0;
    while ( index < (int)table->conflicts.size() )
    {
        int* othact = table->conflicts.data() + index;
        
        bool equal = true;
        for ( int i = 0; i < size; ++i )
        {
            if ( action[ i ] != othact[ i ] )
            {
                equal = false;
                break;
            }
        }
        
        if ( equal )
        {
            return table->state_count + table->rule_count + index;
        }
        
        index += othact[ 0 ];
    }
    
    // No, it doesn't.
    table->conflicts.insert( table->conflicts.end(), action, action + size );
    return table->state_count + table->rule_count + index;
}



void actions::report_conflicts( state* s )
{
    // Detailed reporting of conflicts requires pathfinding through the graph.
    _automata->ensure_distances();
    
    // Group conflicts by whether or not they shift, and by reduction sets.
    for ( size_t i = 0; i < s->actions.size(); )
    {
        action* action = &s->actions.at( i++ );
        if ( action->kind != ACTION_CONFLICT || action->cflict->reported )
        {
            continue;
        }
        
        std::vector< conflict* > conflicts;
        conflicts.push_back( action->cflict );
        action->cflict->reported = true;
        
        for ( size_t j = i; j < s->actions.size(); ++j )
        {
            ::action* similar = &s->actions.at( j++ );
            if ( similar->kind != ACTION_CONFLICT || action->cflict->reported )
            {
                continue;
            }
            
            if ( similar_conflict( action->cflict, similar->cflict ) )
            {
                conflicts.push_back( similar->cflict );
                similar->cflict->reported = true;
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
    conflict* cflict = conflicts.front();

    // If all reductions and shift in conflict are marked, then don't report it.
    bool expected = true;
    for ( conflict* cflict : conflicts )
    {
        if ( cflict->shift && ! cflict->shift->conflicts )
        {
            expected = false;
        }
        for ( reduction* reduce : cflict->reduce )
        {
            if ( ! reduce->drule->conflicts )
            {
                expected = false;
            }
        }
    }

    // Tokens involved in conflict.
    std::string report = "conflict on";
    for ( conflict* conflict : conflicts )
    {
        report += " ";
        report += source->text( conflict->term->name );
    }
    report += " ";
    if ( cflict->shift )
    {
        report += "shift/";
    }
    for ( size_t i = 0; i < cflict->reduce.size(); ++i )
    {
        if ( i != 0 )
        {
            report += "/";
        }
        report += "reduce ";
        report += source->text( cflict->reduce.at( i )->drule->nterm->name );
    }

    srcloc sloc = rule_location( cflict->reduce.front()->drule );
    if ( expected )
    {
        if ( _expected_info )
        {
            _errors->info( sloc, "%s", report.c_str() );
        }
        return;
    }

    _errors->error( sloc, "%s", report.c_str() );

    // Work out which token to use in examples.
    state* next_state = nullptr;
    terminal* next_token = nullptr;
    if ( cflict->shift )
    {
        for ( conflict* conflict : conflicts )
        {
            // Emulate parse with this potential conflict token.
            assert( conflict->shift );
            state* test_state = conflict->shift->next;
            if ( ! next_state || test_state->accept_distance < next_state->accept_distance )
            {
                next_state = test_state;
                next_token = conflict->term;
            }
        }
    }
    else
    {
        next_token = conflicts.front()->term;
    }
    
    // Find left context which successfully supports all conflicting actions.
    parse_search_ptr sp;
    std::vector< std::pair< rule*, parse_search_ptr > > reducep;
    
    left_search_ptr lsearch = std::make_shared< left_search >( _automata, s );
    while ( left_context_ptr lcontext = lsearch->generate() )
    {
        bool success = true;
    
        if ( cflict->shift )
        {
            assert( next_state );
            sp = std::make_shared< parse_search >( _automata, lcontext );
            success = success && sp->shift( next_state, next_token );
        }
        
        reducep.clear();
        for ( reduction* reduce : cflict->reduce )
        {
            auto rp = std::make_shared< parse_search >( _automata, lcontext );
            success = success && rp->reduce( reduce->drule, next_token );
            reducep.emplace_back( reduce->drule, rp );
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

        report.clear();
        report += "shift\n       ";
        sp->print( &report );
        
        srcloc sloc = cflict->shift->tok.sloc;
        _errors->info( sloc, "%s", report.c_str() );
    }
    
    for ( const auto& reduce : reducep )
    {
        reduce.second->search();

        report.clear();
        report += "reduce ";
        report += source->text( reduce.first->nterm->name );
        report += "\n       ";
        reduce.second->print( &report );
        
        _errors->info( rule_location( reduce.first ), "%s", report.c_str() );
    }
}








