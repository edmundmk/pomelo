//
//  parser.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#ifndef PARSER_H
#define PARSER_H


#include "diagnostics.h"
#include "syntax.h"



class parser
{
public:

    parser( diagnostics_ptr diagnostics, syntax_ptr syntax );
    ~parser();
    
    void parse( const char* path );
    
    
private:

    diagnostics_ptr _err;
    syntax_ptr _syntax;

};




#endif



