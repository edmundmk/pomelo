//
//  search.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 29/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include <assert.h>
#include "search.h"


bool left_search::heuristic::operator () ( left_node* a, left_node* b ) const
{
    // Return first open node nearest the target state.
    return a->distance > b->distance;
}




left_search::left_node::left_node( ::state* state )
    :   parent( nullptr )
    ,   child( state->prev.size() )
    ,   state( state )
    ,   symbol( nullptr )
    ,   distance( 0 )
{
}

left_search::left_node::left_node( left_node* parent, transition* trans )
    :   parent( parent )
    ,   child( trans->prev->prev.size() )
    ,   state( trans->prev )
    ,   symbol( trans->symbol )
    ,   distance( parent->distance + 1 )
{
}


left_search::left_search( automata_ptr automata, state* target )
    :   _automata( automata )
    ,   _target( target )
{
    _automata->visited += 1;
}

left_search::~left_search()
{
}


left_context_ptr left_search::generate()
{
    if ( ! _root )
    {
        // Create root node.
        _root = std::make_unique< left_node >( _target );
        _open.push( _root.get() );
    
        // If the target and start nodes are the same, first minimal left
        // context is the empty stack.
        if ( _target == _automata->start )
        {
            return std::make_shared< left_context >();
        }
    }

    while ( _open.size() )
    {
        left_node* node = _open.top();
        _open.pop();
        
        // Find unvisisted open transition with lowest distance.
        size_t index = -1;
        int distance = INT_MAX;
        for ( size_t i = 0; i < node->state->prev.size(); ++i )
        {
            if ( node->child.at( i ) )
            {
                // We have already evaluated this route.
                continue;
            }
            
            transition* trans = node->state->prev.at( i );
            if ( trans->visited != _automata->visited
                 && trans->prev->start_distance < distance )
            {
                index = i;
            }
        }
        
        // Generate shortest route back from the node to the start state.
        if ( index != -1 )
        {
            // This node still had at least one open route.
            _open.push( node );
        
            // Have not gone down this route yet.
            transition* trans = node->state->prev.at( index );
            trans->visited = _automata->visited;

            // Add new node at start of open route.
            auto lnode = std::make_unique< left_node >( node, trans );
            left_node* pnode = lnode.get();
            node->child[ index ] = std::move( lnode );
            _open.push( pnode );

            // Generate shortest route from the new node to the start state.
            return generate_route( pnode );
        }
    }

    // No open nodes remain, we've generated all left contexts.
    return nullptr;
}


left_context_ptr left_search::generate_route( left_node* node )
{
    // Find route that links towards start node.
    while ( node->state != _automata->start )
    {
        for ( size_t i = 0; i < node->state->prev.size(); ++i )
        {
            // There must be a transition that moves closer to the start state.
            transition* trans = node->state->prev.at( i );
            if ( trans->prev->start_distance >= node->state->start_distance )
            {
                continue;
            }
            
            // Add new node.
            auto lnode = std::make_unique< left_node >( node, trans );
            left_node* pnode = lnode.get();
            node->child[ i ] = std::move( lnode );
            _open.push( pnode );
            
            node = pnode;
        }
    }
    
    // Build left context terminating at node.
    left_context_ptr lcontext = std::make_shared< left_context >();
    lcontext->state = _target;
    
    size_t i = 0;
    lcontext->context.resize( node->distance );
    for ( ; node->symbol; node = node->parent )
    {
        lcontext->context[ i++ ] = { node->state, node->symbol };
    }
    assert( i == lcontext->context.size() );
    
    return lcontext;
}




parse_search::value::value( value* prev, ::state* state, ::symbol* symbol )
    :   prev( prev )
    ,   state( state )
    ,   symbol( symbol )
    ,   chead( nullptr )
    ,   ctail( nullptr )
{
}


parse_search::search_head::search_head( value* stack, ::state* state, int distance )
    :   stack( stack )
    ,   state( state )
    ,   distance( distance )
{
}




bool parse_search::heuristic::operator () ( const search_head& a, const search_head& b ) const
{
    int acost = a.distance + a.state->accept_distance;
    int bcost = b.distance + b.state->accept_distance;
    return acost > bcost;
}


parse_search::parse_search( automata_ptr automata, left_context_ptr lcontext )
    :   _automata( automata )
    ,   _left( nullptr, lcontext->state, 0 )
    ,   _accept( nullptr )
{
    // Build parser stack for left context.
    for ( const auto& lv : lcontext->context )
    {
        auto v = std::make_unique< value >( _left.stack, lv.state, lv.symbol );
        _left.stack = v.get();
        _values.push_back( std::move( v ) );
    }
}

parse_search::~parse_search()
{
}



bool parse_search::shift( state* next, terminal* term )
{
    _open.push( shift( _left, next, term ) );
    return true;
}

bool parse_search::reduce( rule* rule, terminal* term )
{
    parse( reduce( _left, rule ), term );
    return ! _open.empty();
}

bool parse_search::search()
{
    const size_t open_limit = 100;
    while ( ! _open.empty() && _open.size() < open_limit )
    {
        // Get lowest-cost parse state.
        search_head head = _open.top();
        _open.pop();
        
        // If the parse state accepts, we are done.
        if ( ! head.state->accept_distance )
        {
            _accept = head.stack->prev;
            return true;
        }
        
        // Evaluate all valid tokens from this state.
        for ( const auto& entry : _automata->syntax->terminals )
        {
            terminal* term = entry.second.get();
            parse( head, term );
        }
        
        // Also 'shift' nonterminals, in case that gets us to the accept state
        // quicker.
        for ( transition* trans : head.state->next )
        {
            if ( trans->symbol->is_terminal )
            {
                continue;
            }
            _open.push( shift( head, trans->next, trans->symbol ) );
        }
        
        // Check if all moves failed and there are no alternatives.
        if ( _open.empty() || _open.size() >= open_limit )
        {
            // Add error marker to end.
            auto v = std::make_unique< value >( head.stack, nullptr, nullptr );
            head.stack = v.get();
            _values.push_back( std::move( v ) );
            
            // 'Reduce' as if it was an accept.
            v = std::make_unique< value >( nullptr, nullptr, nullptr );
            v->chead = head.stack;
            v->ctail = head.stack;
            while ( v->chead->prev )
            {
                v->chead = v->chead->prev;
            }
            head.stack = v.get();
            _values.push_back( std::move( v ) );
            
            // Done.
            _accept = head.stack;
        }
    }
    
    return false;
}


parse_search::search_head parse_search::shift( const search_head& head, state* next, symbol* sym )
{
//    printf( "-->> %s\n", _automata->syntax->source->text( sym->name ) );
    value_ptr v = std::make_unique< value >( head.stack, head.state, sym );
    search_head move( v.get(), next, head.distance + 1 );
    _values.push_back( std::move( v ) );
    return move;
}


parse_search::search_head parse_search::reduce( const search_head& head, rule* rule )
{
 /*
    printf( ">>>> %p %d\n", head.state, head.distance );
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
    for ( transition* trans : valstate->next )
    {
        if ( trans->symbol == rule->nonterminal )
        {
            next_state = trans->next;
            break;
        }
    }
    assert( next_state );
    
    // Create new value and new head.
    value_ptr v = std::make_unique< value >( valprev, valstate, rule->nonterminal );
    v->chead = chead;
    v->ctail = ctail;
    search_head move( v.get(), next_state, head.distance + 1 );
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



void parse_search::print( std::string* out_print )
{
    if ( _accept && _accept->ctail )
    {
        print( _accept->ctail, _accept->chead, out_print );
    }
    else
    {
        out_print->append( "..." );
    }
}

void parse_search::print( const value* tail, const value* head, std::string* out_print )
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
            if ( v->symbol )
            {
                out_print->append( source->text( v->symbol->name ) );
            }
            else
            {
                out_print->append( "..." );
            }
            if ( v == _left.stack )
            {
                out_print->append( " ·" );
            }
        }
    }
}




void parse_search::parse( const search_head& head, terminal* term )
{
    // Perform a parsing step from head.
    search_head move = head;
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

