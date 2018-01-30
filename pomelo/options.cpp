//
//  options.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include "options.h"



options::options()
    :   syntax( false )
    ,   graph( false )
    ,   graph_rgoto( false )
    ,   actions( false )
    ,   resolved_conflicts( false )
{
}

options::~options()
{
}


bool options::parse( int argc, const char* argv[] )
{
    bool result = true;
    bool has_source = false;

    for ( int i = 1; i < argc; ++i )
    {
        const char* arg = argv[ i ];
        if ( strcmp( arg, "--syntax" ) == 0 )
        {
            syntax = true;
        }
        else if ( strcmp( arg, "--graph" ) == 0 )
        {
            graph = true;
        }
        else if ( strcmp( arg, "--graph-rgoto" ) == 0 )
        {
            graph = true;
            graph_rgoto = true;
        }
        else if ( strcmp( arg, "--actions" ) == 0 )
        {
            actions = true;
        }
        else if ( strcmp( arg, "--resolved-conflicts" ) == 0 )
        {
            resolved_conflicts = true;
        }
        else
        {
            if ( ! has_source )
            {
                source = arg;
                has_source = true;
            }
            else
            {
                fprintf( stderr, "additional source file '%s'\n", arg );
                result = false;
            }
        }
    }
    
    if ( ! has_source )
    {
        fprintf( stderr, "no source file provided\n" );
        result = false;
    }
    
    return result;
}

