//
//  errors.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "errors.h"
#include <assert.h>


errors::errors( source_locator* locator, FILE* err )
    :   _err( err )
    ,   _locator( locator )
    ,   _has_error( false )
{
}

errors::~errors()
{
}


void errors::error( srcloc sloc, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    diagnostic( sloc, "error", format, args );
    va_end( args );
    _has_error = true;
}

void errors::warning( srcloc sloc, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    diagnostic( sloc, "warning", format, args );
    va_end( args );
}


bool errors::has_error()
{
    return _has_error;
}



void errors::diagnostic( srcloc sloc, const char* kind, const char* format, va_list args )
{
    file_line fl = _locator->source_location( sloc );
    fprintf
    (
        _err,
        "%s:%d:%d: %s: ",
        fl.file,
        fl.line,
        fl.column,
        kind
    );
    vfprintf
    (
        _err,
        format,
        args
    );
    fputc( '\n', _err );
}
