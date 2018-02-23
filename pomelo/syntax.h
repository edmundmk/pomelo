//
//  syntax.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#ifndef SYNTAX_H
#define SYNTAX_H


#include <limits.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "token.h"


struct directive;
struct location;
struct syntax;
struct symbol;
struct terminal;
struct nonterminal;
struct rule;

typedef std::shared_ptr< syntax > syntax_ptr;
typedef std::unique_ptr< terminal > terminal_ptr;
typedef std::unique_ptr< nonterminal > nonterminal_ptr;
typedef std::unique_ptr< rule > rule_ptr;


enum associativity
{
    ASSOC_NONE,
    ASSOC_LEFT,
    ASSOC_RIGHT,
    ASSOC_NONASSOC,
};


struct directive
{
    directive();

    token           keyword;
    std::string     text;
    bool            specified;
};

struct location
{
    rule*           drule;
    symbol*         sym;
    token           stoken;
    token           sparam;
    bool            conflicts;
};

struct syntax
{
    explicit syntax( source_ptr source );
    ~syntax();
    
    void print();

    source_ptr source;
    directive include;
    directive user_value;
    directive user_split;
    directive class_name;
    directive token_type;
    directive token_prefix;
    directive nterm_prefix;
    directive error_report;
    nonterminal* start;
    std::unordered_map< token, terminal_ptr > terminals;
    std::unordered_map< token, nonterminal_ptr > nonterminals;
    std::vector< rule_ptr > rules;
    std::vector< location > locations;
};

struct symbol
{
    symbol( token name, bool is_terminal );

    token           name;
    int             value           : ( sizeof( int ) * CHAR_BIT ) - 2;
    int             is_special      : 1;
    int             is_terminal     : 1;
};

struct terminal : public symbol
{
    explicit terminal( token name );

    int             precedence      : ( sizeof( int ) * CHAR_BIT ) - 2;
    int             associativity   : 2;
};

struct nonterminal : public symbol
{
    explicit nonterminal( token name );

    std::string     type;
    std::vector< rule* > rules;
    bool            defined;
    bool            erasable;
};

struct rule
{
    explicit rule( nonterminal* nterm );

    nonterminal*    nterm;
    size_t          lostart;
    size_t          locount;
    terminal*       precedence;
    token           precetoken;
    int             index;
    int             actline;
    std::string     action;
    bool            actspecified;
    bool            conflicts;
    bool            reachable;
};



#endif



