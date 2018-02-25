//
//  options.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include "options.h"
#include <string.h>



options::options()
    :   syntax( false )
    ,   dump( false )
    ,   graph( false )
    ,   rgoto( false )
    ,   actions( false )
    ,   conflicts( false )
{
}

options::~options()
{
}


bool options::parse( int argc, const char* argv[] )
{
    bool result = true;
    bool has_source = false;
    bool has_output_c = false;
    bool has_output_h = false;

    for ( int i = 1; i < argc; ++i )
    {
        const char* arg = argv[ i ];
        if ( strcmp( arg, "--syntax" ) == 0 )
        {
            syntax = true;
        }
        else if ( strcmp( arg, "--dump" ) == 0 )
        {
            dump = true;
        }
        else if ( strcmp( arg, "--graph" ) == 0 )
        {
            graph = true;
        }
        else if ( strcmp( arg, "--rgoto" ) == 0 )
        {
            graph = true;
            rgoto = true;
        }
        else if ( strcmp( arg, "--actions" ) == 0 )
        {
            actions = true;
        }
        else if ( strcmp( arg, "--conflicts" ) == 0 )
        {
            conflicts = true;
        }
        else if ( strcmp( arg, "-c" ) == 0 || strcmp( arg, "-h" ) == 0 )
        {
            ++i;
            if ( i < argc )
            {
                const char* path = argv[ i ];
                if ( strcmp( arg, "-c" ) == 0 && ! has_output_c )
                {
                    output_c = path;
                    has_output_c = true;
                }
                else if ( strcmp( arg, "-h" ) == 0 && ! has_output_h )
                {
                    output_h = path;
                    has_output_h = true;
                }
                else
                {
                    fprintf( stderr, "additional output argument '%s'\n", path );
                    result = false;
                }
            }
            else
            {
                fprintf( stderr, "missing output argument\n" );
                result = false;
            }
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

