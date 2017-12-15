//
//  main.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 09/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#include <stdlib.h>
#include "errors.h"
#include "syntax.h"
#include "parser.h"


int main( int argc, const char* argv[] )
{
    const char* path = argv[ 1 ];

    errors_ptr errors = std::make_shared< ::errors >( stderr );
    source_ptr source = std::make_shared< ::source >( path );
    syntax_ptr syntax = std::make_shared< ::syntax >( source );
    parser_ptr parser = std::make_shared< ::parser >( errors, syntax );
    parser->parse( path );

    if ( errors->has_error() )
    {
        return EXIT_FAILURE;
    }
    
    printf( "%%include {%s}\n", syntax->include.text.c_str() );
    printf( "%%class_name {%s}\n", syntax->class_name.text.c_str() );
    printf( "%%token_prefix {%s}\n", syntax->token_prefix.text.c_str() );
    printf( "%%token_type {%s}\n", syntax->token_type.text.c_str() );
    
    for ( const auto& entry : syntax->terminals )
    {
        terminal* tsym = entry.second.get();

        printf
        (
            "%s : %d/%d/%d\n",
            syntax->source->text( tsym->name ),
            tsym->value,
            tsym->precedence,
            tsym->associativity
        );
    }
    
    for ( const auto& entry : syntax->nonterminals )
    {
        nonterminal* nsym = entry.second.get();

        printf
        (
            "%s : %d {%s}\n[\n",
            syntax->source->text( nsym->name ),
            nsym->value,
            nsym->type.c_str()
        );
        
        for ( const auto& rule : nsym->rules )
        {
            printf( "    " );
            for ( const auto& rs : rule->symbols )
            {
                printf( "%s", syntax->source->text( rs.symbol->name ) );
                if ( rs.sparam )
                {
                    printf( "(%s)", syntax->source->text( rs.sparam ) );
                }
                printf( " " );
            }
            printf( ". " );
            if ( rule->precedence )
            {
                terminal* prec = rule->precedence;
                printf
                (
                    "[%s : %d] ",
                    syntax->source->text( prec->name ),
                    prec->precedence
                );
            }
            printf( "{%s}\n", rule->action.c_str() );
        }
        
        printf( "]\n" );
    }

    return EXIT_SUCCESS;
}
