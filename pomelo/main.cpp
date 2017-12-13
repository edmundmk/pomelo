//
//  main.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 09/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#include <stdlib.h>
#include "errors.h"
#include "syntax.h"
#include "parser.h"


int main( int argc, const char* argv[] )
{
    const char* path = argv[ 1 ];

    errors_ptr errors = std::make_shared< ::errors >( stderr );
    source_ptr source = std::make_shared< ::source >( path );
    syntax_ptr syntax = std::make_shared< ::syntax >( source );
    parser_ptr parser = std::make_shared< ::parser >( errors, syntax );
    parser->parse( path );

    return EXIT_SUCCESS;
}
