//
//  syntax.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "syntax.h"


syntax::syntax( source_ptr source )
    :   source( source )
    ,   start( nullptr )
{
}

syntax::~syntax()
{
}


void syntax::print()
{
    
    printf( "%%include {%s}\n", include.text.c_str() );
    printf( "%%class_name {%s}\n", class_name.text.c_str() );
    printf( "%%token_prefix {%s}\n", token_prefix.text.c_str() );
    printf( "%%token_type {%s}\n", token_type.text.c_str() );
    
    for ( const auto& entry : terminals )
    {
        terminal* tsym = entry.second.get();

        printf
        (
            "%s : %d/%d/%d\n",
            source->text( tsym->name ),
            tsym->value,
            tsym->precedence,
            tsym->associativity
        );
    }
    
    for ( const auto& entry : nonterminals )
    {
        nonterminal* nsym = entry.second.get();

        printf
        (
            "%s : %d {%s}\n[\n",
            source->text( nsym->name ),
            nsym->value,
            nsym->type.c_str()
        );
        
        for ( const auto& rule : nsym->rules )
        {
            printf( "    " );
            for ( size_t i = 0; i < rule->locount; ++i )
            {
                const location& l = locations[ rule->lostart + i ];
                if ( l.symbol )
                {
                    printf( "%s", source->text( l.symbol->name ) );
                    if ( l.sparam )
                    {
                        printf( "(%s)", source->text( l.sparam ) );
                    }
                }
                else
                {
                    printf( "." );
                }
                printf( " " );
            }
            if ( rule->precedence )
            {
                terminal* prec = rule->precedence;
                printf
                (
                    "[%s : %d] ",
                    source->text( prec->name ),
                    prec->precedence
                );
            }
            printf( "{%s}\n", rule->action.c_str() );
        }
        
        printf( "]\n" );
    }

}


symbol::symbol( token name, bool is_terminal )
    :   name( name )
    ,   value( -1 )
    ,   is_terminal( is_terminal )
{
}


terminal::terminal( token name )
    :   symbol( name, true )
    ,   precedence( -1 )
    ,   associativity( ASSOC_NONE )
{
}


nonterminal::nonterminal( token name )
    :   symbol( name, false )
    ,   defined( false )
    ,   erasable( false )
{
}


rule::rule( ::nonterminal* nonterminal )
    :   nonterminal( nonterminal )
    ,   lostart( 0 )
    ,   locount( 0 )
    ,   precedence( nullptr )
    ,   precetoken( NULL_TOKEN )
{
}
