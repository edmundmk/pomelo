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
{
}

automata::~automata()
{
}

void automata::print()
{
    // Print out a graphviz dot file.
    printf( "digraph\n{\n" );
    printf( "node [shape=plaintext]\n" );
    for ( const auto& state : states )
    {
        printf( "state%p [label=<<table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n", state.get() );
        for ( size_t i = 0; i < state->closure->size; ++i )
        {
            size_t iloc = state->closure->locations[ i ];
            const location& loc = syntax->locations[ iloc ];
            printf( "<tr><td>%s →", syntax->source->text( loc.rule->nonterminal->name ) );
            for ( size_t i = 0; i < loc.rule->locount; ++i )
            {
                size_t jloc = loc.rule->lostart + i;
                const location& loc = syntax->locations[ jloc ];
                if ( iloc == jloc )
                {
                    printf( " ·" );
                }
                printf( " %s", loc.stoken ? syntax->source->text( loc.stoken ) : "." );
            }
            printf( "</td></tr>\n" );
        }
        printf( "</table>>];\n" );
        
        for ( const auto& transition : state->next )
        {
            if ( transition->rfrom.size() | transition->rgoto.size() )
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
            
            for ( const auto& reducefrom : transition->rgoto )
            {
                printf( "transplit%p -> transplit%p [style=dotted arrowhead=empty];\n", transition, reducefrom->nonterminal );
            }
        }
    }
    printf( "}\n" );
}


