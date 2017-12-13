//
//  errors.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//


#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H


#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <memory>


#define PRINTF_FORMAT( x, y ) __attribute__(( format( printf, x, y ) ))


class errors;
typedef std::shared_ptr< errors > errors_ptr;


typedef size_t srcloc;


class errors
{
public:

    explicit errors( FILE* err );
    ~errors();
    
    void new_file( srcloc sloc, std::string_view name );
    void new_line( srcloc sloc );
    
    void error( srcloc sloc, const char* format, ... ) PRINTF_FORMAT( 3, 4 );
    void warning( srcloc sloc, const char* format, ... ) PRINTF_FORMAT( 3, 4 );
    bool has_error();
    
    
private:

    struct file
    {
        srcloc      sloc;
        std::string name;
        int         line;
    };
    
    void diagnostic( srcloc sloc, const char* kind, const char* format, va_list args );

    FILE*                   _err;
    std::vector< file >     _files;
    std::vector< srcloc >   _lines;
    bool                    _has_error;

};




#endif



