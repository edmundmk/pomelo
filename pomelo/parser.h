//
//  parser.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#ifndef PARSER_H
#define PARSER_H


#include "errors.h"
#include "syntax.h"


class parser;

typedef std::shared_ptr< parser > parser_ptr;



class parser
{
public:

    parser( errors_ptr errors, syntax_ptr syntax );
    ~parser();
    
    void parse( const char* path );
    
    
private:

    static const int TOKEN = -2;
    static const int BLOCK = -3;

    void parse_directive();
    void parse_precedence( associativity associativity );
    void parse_nonterminal();
    void parse_rule( nonterminal* nonterminal );
    
    bool terminal_token( token token );
    symbol* declare_symbol( token token );
    terminal* declare_terminal( token token );
    nonterminal* declare_nonterminal( token token );

    void next();
    void expected( const char* expected );

    errors_ptr _errors;
    syntax_ptr _syntax;
    FILE* _file;
    srcloc _sloc;
    srcloc _tloc;
    int _lexed;
    token _token;
    std::string _block;
    int _precedence;

};




#endif



