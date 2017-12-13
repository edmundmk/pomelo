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
struct syntax;
struct terminal;
struct nonterminal;
struct rule_symbol;
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
};


struct directive
{
    token keyword;
    std::string text;
};

struct syntax
{
    explicit syntax( source_ptr source );
    ~syntax();

    source_ptr source;
    directive include;
    directive class_name;
    directive token_prefix;
    directive token_type;
    std::unordered_map< token, terminal_ptr > terminals;
    std::unordered_map< token, nonterminal_ptr > nonterminals;
};

struct symbol
{
    token name;
    int value           : ( sizeof( int ) * CHAR_BIT ) - 1;
    int terminal        : 1;
};

struct terminal : public symbol
{
    int precedence      : ( sizeof( int ) * CHAR_BIT ) - 2;
    int associativity   : 2;
};

struct nonterminal : public symbol
{
    std::string type;
    std::vector< rule_ptr > rules;
};

struct rule_symbol
{
    symbol* symbol;
    token stoken;
    token sparam;
};

struct rule
{
    nonterminal* nonterminal;
    std::vector< rule_symbol > symbols;
    terminal* precedence;
    std::string action;
};



#endif



