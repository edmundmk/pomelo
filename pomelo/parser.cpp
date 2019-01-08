//
//  parser.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//

#include "parser.h"
#include <stdio.h>
#include <string.h>


parser::parser( errors_ptr errors, syntax_ptr syntax )
    :   _errors( errors )
    ,   _syntax( syntax )
    ,   _file( nullptr )
    ,   _sloc( 0 )
    ,   _tloc( 0 )
    ,   _lexed( EOF )
    ,   _token( NULL_TOKEN )
    ,   _precedence( 0 )
{
}

parser::~parser()
{
}


void parser::parse( const char* path )
{
    // Open file.
    srcloc sloc = 0;
    _file = fopen( path, "r" );
    if ( ! _file )
    {
        _errors->error( sloc, "unable to open file '%s'", path );
        return;
    }
    
    // Parse source file.
    next();
    while ( 1 )
    {
        if ( _lexed == '%' )
        {
            parse_directive();
        }
        else if ( _lexed == TOKEN )
        {
            parse_nonterminal();
        }
        else if ( _lexed == EOF )
        {
            break;
        }
        else
        {
            expected( nullptr );
            next();
        }
    }
    
    fclose( _file );
    
    // Check for a valid start symbol.
    if ( _syntax->start )
    {
        // Start symbol is special.
        token start_token = _syntax->source->new_token( 0, "$start" );
        nonterminal_ptr start = std::make_unique< nonterminal >( start_token );
        start->is_special   = true;
        start->type         = _syntax->start->type;
        
        token eoi_token = _syntax->source->new_token( 0, "$EOI" );
        terminal_ptr eoi = std::make_unique< terminal >( eoi_token );
        eoi->is_special     = true;
        
        rule_ptr rule = std::make_unique< ::rule >( start.get() );
        rule->lostart       = _syntax->locations.size();
        rule->locount       = 3;
        
        _syntax->locations.push_back( { rule.get(), _syntax->start, start->name, NULL_TOKEN } );
        _syntax->locations.push_back( { rule.get(), eoi.get(), eoi->name, NULL_TOKEN } );
        _syntax->locations.push_back( { rule.get(), nullptr, NULL_TOKEN, NULL_TOKEN } );
        
        start->rules.push_back( rule.get() );
        _syntax->rules.push_back( std::move( rule ) );
        
        _syntax->start = start.get();
        _syntax->terminals.emplace( eoi->name, std::move( eoi ) );
        _syntax->nonterminals.emplace( start->name, std::move( start ) );
    }
    else
    {
        _errors->error( 0, "no start symbol defined" );
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
    
    // Give all symbols a value.
    std::vector< symbol* > symbols;
    for ( const auto& tsym : _syntax->terminals )
    {
        symbols.push_back( tsym.second.get() );
    }
    for ( const auto& nsym : _syntax->nonterminals )
    {
        symbols.push_back( nsym.second.get() );
    }
    std::sort
    (
        symbols.begin(),
        symbols.end(),
        []( symbol* a, symbol* b )
        {
            if ( a->is_terminal != b->is_terminal )
            {
                return a->is_terminal ? true : false;
            }
            else
            {
                return a->name.sloc < b->name.sloc;
            }
        }
    );
    int value = 0;
    for ( symbol* sym : symbols )
    {
        sym->value = value++;
    }
}


void parser::parse_directive()
{
    srcloc dloc = _tloc;
    next();
    
    if ( _lexed != TOKEN )
    {
        expected( "directive" );
        return;
    }
    
    const char* text = _syntax->source->text( _token );
    directive* directive = nullptr;
    if ( strcmp( text, "include" ) == 0 || strcmp( text, "include_header" ) == 0 )
    {
        directive = &_syntax->include_header;
    }
    else if ( strcmp( text, "include_source" ) == 0 )
    {
        directive = &_syntax->include_source;
    }
    else if ( strcmp( text, "user_value" ) == 0 )
    {
        directive = &_syntax->user_value;
    }
    else if ( strcmp( text, "user_split" ) == 0 )
    {
        directive = &_syntax->user_split;
    }
    else if ( strcmp( text, "class_name" ) == 0 )
    {
        directive = &_syntax->class_name;
    }
    else if ( strcmp( text, "token_type" ) == 0 )
    {
        directive = &_syntax->token_type;
    }
    else if ( strcmp( text, "token_prefix" ) == 0 )
    {
        directive = &_syntax->token_prefix;
    }
    else if ( strcmp( text, "nterm_prefix" ) == 0 )
    {
        directive = &_syntax->nterm_prefix;
    }
    else if ( strcmp( text, "error_report" ) == 0 )
    {
        directive = &_syntax->error_report;
    }
    else if ( strcmp( text, "left" ) == 0 )
    {
        parse_precedence( ASSOC_LEFT );
        return;
    }
    else if ( strcmp( text, "right" ) == 0 )
    {
        parse_precedence( ASSOC_RIGHT );
        return;
    }
    else if ( strcmp( text, "nonassoc" ) == 0 )
    {
        parse_precedence( ASSOC_NONASSOC );
        return;
    }
    else
    {
        expected( "directive" );
    }
    
    if ( directive->specified )
    {
        _errors->error( dloc, "repeated directive '%%%s'", text );
    }
    
    directive->keyword = _token;
    
    next();
    
    if ( _lexed != BLOCK )
    {
        expected( "code block" );
        return;
    }

    directive->text = _block.size() ? _block : " ";
    directive->specified = true;
    next();
}


void parser::parse_precedence( associativity associativity )
{
    int precedence = _precedence++;
    
    next();
    while ( true )
    {
        if ( _lexed == TOKEN )
        {
            terminal* terminal = declare_terminal( _token );
            terminal->associativity = associativity;
            terminal->precedence = precedence;
            next();
        }
        else if ( _lexed == '.' )
        {
            next();
            break;
        }
        else
        {
            expected( "terminal symbol" );
            break;
        }
    }
}

void parser::parse_nonterminal()
{
    nonterminal* nonterminal = declare_nonterminal( _token );
    if ( ! _syntax->start )
    {
        _syntax->start = nonterminal;
    }

    if ( nonterminal->defined )
    {
        const char* name = _syntax->source->text( nonterminal->name );
        _errors->error( nonterminal->name.sloc, "nonterminal '%s' has already been defined", name );
    }
    nonterminal->name = _token;
    nonterminal->defined = true;

    next();
    if ( _lexed == BLOCK )
    {
        nonterminal->type = _block;
        next();
    }

    if ( _lexed == '@' )
    {
        next();
        if ( _lexed != BLOCK )
        {
            expected( "merge code" );
            return;
        }

        file_line line = _syntax->source->source_location( _tloc );
        nonterminal->gline = line.line;        
        nonterminal->gmerge = _block;
        nonterminal->gspecified = true;
        next();
    }
    
    if ( _lexed != '[' )
    {
        expected( "'['" );
        return;
    }
    
    next();
    while ( true )
    {
        if ( _lexed == '!' || _lexed == TOKEN || _lexed == '.' )
        {
            parse_rule( nonterminal );
        }
        else if ( _lexed == ']' )
        {
            next();
            break;
        }
        else
        {
            expected( "rule or ']'" );
            if ( _lexed == EOF )
            {
                break;
            }
            next();
        }
    }
    
    if ( nonterminal->rules.empty() )
    {
        const char* name = _syntax->source->text( nonterminal->name );
        _errors->error( nonterminal->name.sloc, "nonterminal '%s' has no rules", name );
    }
}

void parser::parse_rule( nonterminal* nonterminal )
{
    rule_ptr rule = std::make_unique< ::rule >( nonterminal );
    rule->lostart = _syntax->locations.size();
    rule->locount = 0;

    while ( true )
    {
        bool conflicts = false;
        if ( _lexed == '!' )
        {
            conflicts = true;
            next();
        }
    
        if ( _lexed == TOKEN )
        {
            symbol* symbol = declare_symbol( _token );
            location l = { rule.get(), symbol, _token, NULL_TOKEN, conflicts };
            
            if ( l.sym->is_terminal && ! rule->precedence )
            {
                rule->precedence = (terminal*)l.sym;
                rule->precetoken = _token;
            }
            
            next();
            if ( _lexed == '(' )
            {
                next();
                if ( _lexed == TOKEN )
                {
                    l.sparam = _token;

                    next();
                    if ( _lexed == ')' )
                    {
                        next();
                    }
                    else
                    {
                        expected( "')'" );
                    }
                }
                else
                {
                    expected( "parameter name" );
                }
            }

            _syntax->locations.push_back( l );
            rule->locount += 1;
        }
        else if ( _lexed == '.' )
        {
            location l = { rule.get(), nullptr, _token, NULL_TOKEN, false };
            _syntax->locations.push_back( l );
            rule->locount += 1;
            rule->conflicts = conflicts;

            next();
            break;
        }
        else
        {
            expected( "symbol or '.'" );
            if ( _lexed == ']' || _lexed == EOF )
            {
                break;
            }
            next();
        }
    }

    if ( _lexed == '[' )
    {
        next();
        if ( _lexed == TOKEN )
        {
            rule->precedence = declare_terminal( _token );
            rule->precetoken = _token;

            next();
            if ( _lexed == ']' )
            {
                next();
            }
            else
            {
                expected( "']'" );
            }
        }
        else
        {
            expected( "terminal precdence symbol" );
        }
    }

    if ( _lexed == BLOCK )
    {
        file_line line = _syntax->source->source_location( _tloc );
        rule->actline = line.line;
        rule->action = _block;
        rule->actspecified = true;
        next();
    }

    nonterminal->rules.push_back( rule.get() );
    _syntax->rules.push_back( std::move( rule ) );
}



bool parser::terminal_token( token token )
{
    const char* text = _syntax->source->text( token );
    return isupper( text[ 0 ] );
}

symbol* parser::declare_symbol( token token )
{
    if ( terminal_token( token ) )
    {
        return declare_terminal( token );
    }
    else
    {
        return declare_nonterminal( token );
    }
}

terminal* parser::declare_terminal( token token )
{
    auto i = _syntax->terminals.find( token );
    if ( i != _syntax->terminals.end() )
    {
        return i->second.get();
    }
    
    terminal_ptr usym = std::make_unique< terminal >( token );
    terminal* psym = usym.get();
    _syntax->terminals.emplace( token, std::move( usym ) );
    return psym;
}

nonterminal* parser::declare_nonterminal( token token )
{
    auto i = _syntax->nonterminals.find( token );
    if ( i != _syntax->nonterminals.end() )
    {
        return i->second.get();
    }
    
    nonterminal_ptr usym = std::make_unique< nonterminal >( token );
    nonterminal* psym = usym.get();
    _syntax->nonterminals.emplace( token, std::move( usym ) );
    return psym;
}



void parser::next()
{
    while ( true )
    {
        int c = fgetc( _file );
        _tloc = _sloc;
        _sloc += 1;

        if ( c == ' ' || c == '\t' )
        {
            continue;
        }
        else if ( c == '\r' )
        {
            c = fgetc( _file );
            _sloc += 1;
            
            if ( c != '\n' )
            {
                ungetc( c, _file );
                _sloc -= 1;
            }
        
            _syntax->source->new_line( _sloc );
            continue;
        }
        else if ( c == '\n' )
        {
            _syntax->source->new_line( _sloc );
            continue;
        }
        else if ( c == '/' )
        {
            c = fgetc( _file );
            _sloc += 1;

            if ( c == '*' )
            {
                bool was_asterisk = false;
                while ( ! was_asterisk || c != '/' )
                {
                    was_asterisk = ( c == '*' );
                    c = fgetc( _file );
                    _sloc += 1;
                    
                    if ( c == '\r' )
                    {
                        c = fgetc( _file );
                        _sloc += 1;
                        
                        if ( c != '\n' )
                        {
                            ungetc( c, _file );
                            _sloc -= 1;
                        }
                    
                        _syntax->source->new_line( _sloc );
                    }
                    else if ( c == '\n' )
                    {
                        _syntax->source->new_line( _sloc );
                    }
                    else if ( c == EOF )
                    {
                        _errors->error( _tloc, "unterminated block comment" );
                        break;
                    }
                }
                continue;
            }
            else if ( c == '/' )
            {
                while ( c != '\r' && c != '\n' && c != EOF )
                {
                    c = fgetc( _file );
                    _sloc += 1;
                }
                
                ungetc( c, _file );
                _sloc -= 1;
                continue;
            }
            else
            {
                ungetc( c, _file );
                _sloc -= 1;
                
                _lexed = '/';
                return;
            }
        }
        else if ( c == '{' )
        {
            _block.clear();
            size_t brace_depth = 1;
            while ( true )
            {
                c = fgetc( _file );
                _sloc += 1;
            
                if ( c == '{' )
                {
                    brace_depth += 1;
                }
                if ( c == '}' )
                {
                    brace_depth -= 1;
                    if ( ! brace_depth )
                    {
                        break;
                    }
                }
                else if ( c == '\r' )
                {
                    c = fgetc( _file );
                    _sloc += 1;
                
                    if ( c != '\n' )
                    {
                        ungetc( c, _file );
                        _sloc -= 1;
                    }
                    else
                    {
                        _block.push_back( '\r' );
                    }
                
                    _syntax->source->new_line( _sloc );
                }
                else if ( c == '\n' )
                {
                    _syntax->source->new_line( _sloc );
                }
                else if ( c == EOF )
                {
                    _errors->error( _tloc, "unterminated code block" );
                    break;
                }
                
                _block.push_back( c );
            }
            
            _lexed = BLOCK;
            return;
        }
        else if ( c == '_'
                || ( c >= 'a' && c <= 'z' )
                || ( c >= 'A' && c <= 'Z' ) )
        {
            _block.clear();
            while ( c == '_'
                    || ( c >= 'a' && c <= 'z' )
                    || ( c >= 'A' && c <= 'Z' )
                    || ( c >= '0' && c <= '9' ) )
            {
                _block.push_back( c );
                c = fgetc( _file );
                _sloc += 1;
            }
            
            ungetc( c, _file );
            _sloc -= 1;
            
            _lexed = TOKEN;
            _token = _syntax->source->new_token( _tloc, _block );
            return;
        }
        else if ( c == '.' )
        {
            _lexed = '.';
            _token = _syntax->source->new_token( _tloc, "." );
            return;
        }
        else
        {
            _lexed = c;
            return;
        }
    }
}

void parser::expected( const char* expected )
{
    std::string message;
    if ( expected )
    {
        message = "expected ";
        message += expected;
        message += ", not ";
    }
    else
    {
        message = "unexpected ";
    }
    
    if ( _lexed == TOKEN )
    {
        message += "'";
        message += _syntax->source->text( _token );
        message += "'";
    }
    else if ( _lexed == BLOCK )
    {
        message += "code block";
    }
    else
    {
        message += "'";
        message.push_back( _lexed );
        message += "'";
    }
    
    _errors->error( _tloc, "%s", message.c_str() );
}











