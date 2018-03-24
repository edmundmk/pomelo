//
//  main.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 09/12/2017.
//  Copyright © 2017 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#include <stdlib.h>
#include "options.h"
#include "errors.h"
#include "syntax.h"
#include "parser.h"
#include "lalr1.h"
#include "actions.h"
#include "write.h"


int main( int argc, const char* argv[] )
{
    options options;
    if ( ! options.parse( argc, argv ) )
    {
        return EXIT_FAILURE;
    }

    source_ptr source = std::make_shared< ::source >( options.source.c_str() );
    errors_ptr errors = std::make_shared< ::errors >( source.get(), stderr );
    syntax_ptr syntax = std::make_shared< ::syntax >( source );
    parser_ptr parser = std::make_shared< ::parser >( errors, syntax );
    parser->parse( options.source.c_str() );

    if ( options.syntax )
    {
        syntax->print();
    }

    if ( errors->has_error() )
    {
        return EXIT_FAILURE;
    }
    
    lalr1_ptr lalr1 = std::make_shared< ::lalr1 >( errors, syntax );
    automata_ptr automata = lalr1->construct();
    
    if ( options.graph )
    {
        automata->print_graph( options.rgoto );
    }
    
    if ( errors->has_error() )
    {
        return EXIT_FAILURE;
    }
    
    actions_ptr actions = std::make_shared< ::actions >( errors, automata, options.conflicts );
    actions->analyze();
    
    if ( options.actions )
    {
        actions->print();
    }

    if ( options.dump )
    {
        automata->print_dump();
    }
    
    actions->report_conflicts();
    
    if ( errors->has_error() )
    {
        return EXIT_FAILURE;
    }
    
    action_table_ptr action_table = actions->build_action_table();
    goto_table_ptr goto_table = actions->build_goto_table();
    
    write_ptr write = std::make_shared< ::write >
        (
            automata,
            action_table,
            goto_table,
            options.source,
            options.output_h
        );
    write->prepare();
    
    bool ok = true;
    if ( options.output_h.size() )
    {
        FILE* header = fopen( options.output_h.c_str(), "w" );
        if ( ! header || ! write->write_header( header ) )
        {
            ok = false;
        }
        if ( header )
        {
            fclose( header );
        }
    }
    else
    {
        if ( ! write->write_header( stdout ) )
        {
            ok = false;
        }
    }
        
    if ( options.output_c.size() )
    {
        FILE* source = fopen( options.output_c.c_str(), "w" );
        if ( ! source || ! write->write_source( source ) )
        {
            ok = false;
        }
        if ( source )
        {
            fclose( source );
        }
    }
    else
    {
        if ( ! write->write_source( stdout ) )
        {
            ok = false;
        }
    }
    
    if ( ! ok )
    {
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
