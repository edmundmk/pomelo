//
//  errors.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "errors.h"
#include <assert.h>


errors::errors( FILE* err )
    :   _err( err )
    ,   _has_error( false )
{
}

errors::~errors()
{
}


void errors::new_file( srcloc sloc, std::string_view name )
{
    assert( _files.empty() || _files.back().sloc < sloc );
    _files.push_back( { sloc, std::string( name ), (int)_lines.size() } );
    _lines.push_back( sloc );
}

void errors::new_line( srcloc sloc )
{
    assert( ! _files.empty() && _files.back().sloc < sloc );
    assert( ! _lines.empty() && _lines.back() < sloc );
    _lines.push_back( sloc );
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
    assert( ! _files.empty() );
    assert( ! _lines.empty() );

    auto f = std::upper_bound
    (
        _files.begin(),
        _files.end(),
        sloc,
        []( srcloc a, const file& b ) { return a < b.sloc; }
    );
    f--;
    
    auto l = std::upper_bound
    (
        _lines.begin(),
        _lines.end(),
        sloc
    );
    l--;
    
    int line = (int)( l - _lines.begin() ) + 1;
    int column = (int)( sloc - *l );
    
    fprintf
    (
        _err,
        "%s:%d:%d: %s: ",
        f->name.c_str(),
        line,
        column,
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
