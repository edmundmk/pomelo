//
//  syntax.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#ifndef SYNTAX_H
#define SYNTAX_H


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
    token           keyword;
    std::string     text;
};

struct location
{
    rule*           rule;
    symbol*         symbol;
    token           stoken;
    token           sparam;
};

struct syntax
{
    explicit syntax( source_ptr source );
    ~syntax();
    
    void print();

    source_ptr source;
    directive include;
    directive class_name;
    directive token_type;
    directive token_prefix;
    directive nterm_prefix;
    nonterminal* start;
    std::unordered_map< token, terminal_ptr > terminals;
    std::unordered_map< token, nonterminal_ptr > nonterminals;
    std::vector< location > locations;
};

struct symbol
{
    symbol( token name, bool is_terminal );

    token           name;
    int             value           : ( sizeof( int ) * CHAR_BIT ) - 1;
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
    std::vector< rule_ptr > rules;
    bool            defined;
    bool            erasable;
};

struct rule
{
    explicit rule( nonterminal* nonterminal );

    nonterminal*    nonterminal;
    size_t          lostart;
    size_t          locount;
    terminal*       precedence;
    token           precetoken;
    std::string     action;
};



#endif



