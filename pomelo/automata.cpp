//
//  automata.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 17/12/2017.
//  Copyright © 2017 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#include "automata.h"
#include <limits.h>


/*
    Deletes closure pointers using free().
*/

void closure_deleter::operator () ( closure* p ) const
{
    free( p );
}




/*
    Automata.
*/

automata::automata( syntax_ptr syntax )
    :   syntax( syntax )
    ,   start( nullptr )
    ,   accept( nullptr )
    ,   visited( 0 )
    ,   rule_count( -1 )
    ,   state_count( -1 )
    ,   has_distances( false )
{
}

automata::~automata()
{
}


static void traverse_start( state* s, int distance )
{
    if ( s->start_distance <= distance )
    {
        return;
    }
    
    s->start_distance = distance;
    distance += 1;
    for ( transition* trans : s->next )
    {
        traverse_start( trans->next, distance );
    }
}


static void traverse_accept( state* s, int distance )
{
    if ( s->accept_distance <= distance )
    {
        return;
    }
    
    s->accept_distance = distance;
    distance += 1;
    for ( transition* trans : s->prev )
    {
        traverse_accept( trans->prev, distance );
        for ( const auto& rfrom : trans->rfrom )
        {
            traverse_accept( rfrom->finalsymbol->next, distance );
        }
    }
}


void automata::ensure_distances()
{
    if ( has_distances )
    {
        return;
    }
    
    // Traverse DFA from start state to work out distance from the start.
    traverse_start( start, 0 );

    // Traverse DFA from accept state (and following rfrom links) to work
    // out distance to the accept state.
    traverse_accept( accept, 0 );
}


void automata::print_graph( bool rgoto )
{
    // Print out a graphviz dot file.
    printf( "digraph\n{\n" );
    printf( "node [shape=plaintext]\n" );
    for ( const auto& state : states )
    {
        bool start_accept = state.get() == start || state.get() == accept;
        printf( "state%p [label=<<table border=\"%d\" cellborder=\"1\" cellspacing=\"0\">\n", state.get(), start_accept ? 4 : 0 );
        for ( size_t i = 0; i < state->closure->size; ++i )
        {
            size_t iloc = state->closure->locations[ i ];
            const location& loc = syntax->locations[ iloc ];
            printf( "<tr><td>%s →", syntax->source->text( loc.drule->nterm->name ) );
            for ( size_t i = 0; i < loc.drule->locount - 1; ++i )
            {
                size_t jloc = loc.drule->lostart + i;
                const location& loc = syntax->locations[ jloc ];
                if ( iloc == jloc )
                {
                    printf( " ·" );
                }
                printf( " %s", syntax->source->text( loc.stoken ) );
            }
            if ( ! loc.sym )
            {
                for ( const auto& reduction : state->reductions )
                {
                    printf( " [" );
                    for ( const auto& terminal : reduction->lookahead )
                    {
                        printf( " %s", syntax->source->text( terminal->name ) );
                    }
                    printf( " ]" );
                }
            }
            printf( "</td></tr>\n" );
        }
        printf( "</table>>];\n" );
        
        for ( transition* trans : state->next )
        {
            if ( rgoto && ( trans->rfrom.size() | trans->rgoto.size() ) )
            {
                // Need to split this transition.
                printf( "transplit%p [label=\"\", fixedsize=\"false\", width=0, height=0, shape=none];\n", trans );
                printf
                (
                    "state%p -> transplit%p [label=\"%s\" arrowhead=none];\n",
                    trans->prev,
                    trans,
                    syntax->source->text( trans->sym->name )
                );
                printf( "transplit%p -> state%p;\n", trans, trans->next );
            }
            else
            {
                printf
                (
                    "state%p -> state%p [label=\"%s\"]\n",
                    trans->prev,
                    trans->next,
                    syntax->source->text( trans->sym->name )
                );
            }
            
            if ( rgoto )
            {
                for ( reducefrom* rfrom : trans->rgoto )
                {
                    printf( "transplit%p -> transplit%p [style=dotted arrowhead=empty];\n", trans, rfrom->nonterminal );
                }
            }
        }
    }
    printf( "}\n" );
}

void automata::print_dump()
{
    for ( const auto& state : states )
    {
        printf( "STATE %d\n", state->index );
        for ( size_t i = 0; i < state->closure->size; ++i )
        {
            size_t iloc = state->closure->locations[ i ];
            const location& loc = syntax->locations[ iloc ];
            printf( "%s →", syntax->source->text( loc.drule->nterm->name ) );
            for ( size_t i = 0; i < loc.drule->locount - 1; ++i )
            {
                size_t jloc = loc.drule->lostart + i;
                const location& loc = syntax->locations[ jloc ];
                if ( iloc == jloc )
                {
                    printf( " ·" );
                }
                printf( " %s", syntax->source->text( loc.stoken ) );
            }
            if ( ! loc.sym )
            {
                for ( const auto& reduction : state->reductions )
                {
                    printf( " [" );
                    for ( const auto& terminal : reduction->lookahead )
                    {
                        printf( " %s", syntax->source->text( terminal->name ) );
                    }
                    printf( " ]" );
                }
            }
            printf( "\n" );
        }
        for ( transition* trans : state->next )
        {
            printf
            (
                "    %s -> %d\n",
                syntax->source->text( trans->sym->name ),
                trans->next->index
            );
        }
        printf( "\n" );
    }
}



conflict::conflict( terminal* term )
    :   term( term )
    ,   shift( nullptr )
    ,   reported( false )
{
}


state::state( closure_ptr&& closure )
    :   closure( std::move( closure ) )
    ,   visited( 0 )
    ,   start_distance( INT_MAX )
    ,   accept_distance( INT_MAX )
    ,   index( -1 )
    ,   has_conflict( false )
    ,   reachable( false )
{
}


transition::transition( state* prev, state* next, symbol* nsym, token tok, bool conflicts )
    :   prev( prev )
    ,   next( next )
    ,   sym( nsym )
    ,   tok( tok )
    ,   visited( 0 )
    ,   conflicts( conflicts )
{
}


reducefrom::reducefrom( rule* drule, transition* nonterminal, transition* finalsymbol )
    :   drule( drule )
    ,   nonterminal( nonterminal )
    ,   finalsymbol( finalsymbol )
{
}


reduction::reduction( rule* drule )
    :   drule( drule )
{
}

