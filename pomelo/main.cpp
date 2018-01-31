//
//  main.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 09/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#include <stdlib.h>
#include "options.h"
#include "errors.h"
#include "syntax.h"
#include "parser.h"
#include "lalr1.h"
#include "actions.h"


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
        automata->print( options.rgoto );
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
    
    actions->report_conflicts();
    
    if ( errors->has_error() )
    {
        return EXIT_FAILURE;
    }
    
    action_table_ptr action_table = actions->build_action_table();
    goto_table_ptr goto_table = actions->build_goto_table();
    
    printf
    (
        "ACTS : %dx%d %d (%d)\n",
        action_table->token_count,
        action_table->state_count,
        action_table->token_count * action_table->state_count,
        action_table->max_conflict
    );
    
    printf
    (
        "GOTO : %dx%d %d (%d)\n",
        goto_table->nterm_count,
        goto_table->state_count,
        goto_table->nterm_count * goto_table->state_count,
        goto_table->max_state
    );
    
    return EXIT_SUCCESS;
}
