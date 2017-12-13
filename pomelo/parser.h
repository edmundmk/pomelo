//
//  parser.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
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

    struct lexed { srcloc sloc; int c; token token; };

    lexed lex();
    std::string lex_block();

    void parse_directive();
    void parse_precedence( associativity associativity );
    void parse_nonterminal( token token );
    

    errors_ptr _errors;
    syntax_ptr _syntax;
    FILE* _file;
    srcloc _sloc;
    int _precedence;

};




#endif



