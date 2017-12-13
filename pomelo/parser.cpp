//
//  parser.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "parser.h"
#include <stdio.h>


parser::parser( errors_ptr errors, syntax_ptr syntax )
    :   _errors( errors )
    ,   _syntax( syntax )
{
}

parser::~parser()
{
}


void parser::parse( const char* path )
{
    // Open file.
    srcloc sloc = 0;
    _errors->new_file( sloc, path );
    FILE* f = fopen( path, "r" );
    if ( ! f )
    {
        _errors->error( sloc, "unable to open file '%s'", path );
        return;
    }
    
    // Parse source file.
    while ( 1 )
    {
        lexed l = lex();
        if ( l.c == '%' )
        {
            parse_directive();
        }
        else if ( l.c == 0 )
        {
            parse_nonterminal( l.token );
        }
        else if ( l.c == EOF )
        {
            break;
        }
        else
        {
            _errors->error( l.sloc, "unexpected character '%c'", l.c );
        }
    }

    // Check that all nonterminals have been defined.
    for ( const auto& entry : _syntax->nonterminals )
    {
        nonterminal* nterm = entry.second.get();
        if ( nterm->rules.empty() )
        {
            _errors->error
            (
                nterm->name.sloc,
                "nonterminal '%s' has not been defined",
                _syntax->source->text( nterm->name )
            );
        }
    }
}


void parser::parse_directive()
{
    lexed l = lex();
    if ( l.c )
    {
        _errors->error( l.sloc, "expected directive, not '%c'", l.c );
        return;
    }
    
    const char* text = _syntax->source->text( l.token );
    directive* directive = nullptr;
    if ( strcmp( text, "include" ) == 0 )
    {
        directive = &_syntax->include;
    }
    else if ( strcmp( text, "class_name" ) == 0 )
    {
        directive = &_syntax->class_name;
    }
    else if ( strcmp( text, "token_prefix" ) == 0 )
    {
        directive = &_syntax->token_prefix;
    }
    else if ( strcmp( text, "token_type" ) == 0 )
    {
        directive = &_syntax->token_type;
    }
    else if ( strcmp( text, "left" ) == 0 )
    {
        parse_precedence( ASSOC_LEFT );
    }
    else if ( strcmp( text, "right" ) == 0 )
    {
        parse_precedence( ASSOC_RIGHT );
    }
    else if ( strcmp( text, "nonassoc" ) == 0 )
    {
        parse_precedence( ASSOC_NONE );
    }
    else
    {
        _errors->error( l.sloc, "expected directive, not '%s'", text );
    }
    
    if ( directive )
    {
        directive->keyword = l.token;
        directive->text = lex_block();
    }
}


void parser::parse_precedence( associativity associativity )
{
}

void parser::parse_nonterminal( token token )
{
}




