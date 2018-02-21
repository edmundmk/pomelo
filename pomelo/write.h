//
//  write.h
//  pomelo
//
//  Created by Edmund Kapusniak on 02/02/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#ifndef WRITE_H
#define WRITE_H


#include "actions.h"


class write;
typedef std::shared_ptr< write > write_ptr;


class write
{
public:

    write( automata_ptr automata, action_table_ptr action_table,
            goto_table_ptr goto_table, const std::string& output );
    ~write();

    void prepare();
    bool write_header( FILE* f );
    bool write_source( FILE* f );


private:

    struct ntype
    {
        std::string ntype;
        int value;
    };

    std::string trim( const std::string& s );
    bool starts_with( const std::string& s, const std::string& z );
        
    bool write_template( FILE* f, char* source, size_t length, bool header );
    std::string replace( std::string line );
    std::string replace( std::string line, terminal* token );
    std::string replace( std::string line, nonterminal* nterm );
    std::string replace( std::string line, ntype* ntype );
    std::string replace( std::string line, rule* rule, bool header );
    std::string write_table( const std::vector< int >& table );
    std::string write_rule_table();

    errors_ptr _errors;
    automata_ptr _automata;
    action_table_ptr _action_table;
    goto_table_ptr _goto_table;
    std::string _output_h;
    std::vector< terminal* > _tokens;
    std::vector< nonterminal* > _nterms;
    std::unordered_map< nonterminal*, ntype* > _nterm_lookup;
    std::vector< std::unique_ptr< ntype > > _ntypes;

};


#endif


