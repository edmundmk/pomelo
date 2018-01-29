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




