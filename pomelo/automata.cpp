//
//  automata.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 17/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "automata.h"



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
{
}

automata::~automata()
{
}

void automata::print( bool rgoto )
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
            printf( "<tr><td>%s →", syntax->source->text( loc.rule->nonterminal->name ) );
            for ( size_t i = 0; i < loc.rule->locount - 1; ++i )
            {
                size_t jloc = loc.rule->lostart + i;
                const location& loc = syntax->locations[ jloc ];
                if ( iloc == jloc )
                {
                    printf( " ·" );
                }
                printf( " %s", syntax->source->text( loc.stoken ) );
            }
            if ( ! loc.symbol )
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
        
        for ( transition* transition : state->next )
        {
            if ( rgoto && ( transition->rfrom.size() | transition->rgoto.size() ) )
            {
                // Need to split this transition.
                printf( "transplit%p [label=\"\", fixedsize=\"false\", width=0, height=0, shape=none];\n", transition );
                printf
                (
                    "state%p -> transplit%p [label=\"%s\" arrowhead=none];\n",
                    transition->prev,
                    transition,
                    syntax->source->text( transition->symbol->name )
                );
                printf( "transplit%p -> state%p;\n", transition, transition->next );
            }
            else
            {
                printf
                (
                    "state%p -> state%p [label=\"%s\"]\n",
                    transition->prev,
                    transition->next,
                    syntax->source->text( transition->symbol->name )
                );
            }
            
            if ( rgoto )
            {
                for ( ::reducefrom* reducefrom : transition->rgoto )
                {
                    printf( "transplit%p -> transplit%p [style=dotted arrowhead=empty];\n", transition, reducefrom->nonterminal );
                }
            }
        }
    }
    printf( "}\n" );
}


conflict::conflict( ::terminal* terminal )
    :   terminal( terminal )
    ,   shift( nullptr )
    ,   reported( false )
{
}


state::state( closure_ptr&& closure )
    :   closure( std::move( closure ) )
    ,   visited( 0 )
    ,   start_distance( INT_MAX )
    ,   accept_distance( INT_MAX )
    ,   has_conflict( false )
{
}


transition::transition( state* prev, state* next, ::symbol* nsym )
    :   prev( prev )
    ,   next( next )
    ,   symbol( nsym )
    ,   visited( 0 )
{
}


reducefrom::reducefrom( ::rule* rule, transition* nonterminal, transition* finalsymbol )
    :   rule( rule )
    ,   nonterminal( nonterminal )
    ,   finalsymbol( finalsymbol )
{
}


reduction::reduction( ::rule* rule )
    :   rule( rule )
{
}

