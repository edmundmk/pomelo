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


struct file_line;
struct source_locator;
class errors;
typedef std::shared_ptr< errors > errors_ptr;


typedef size_t srcloc;


struct file_line
{
    const char* file;
    int line;
    int column;
};


struct source_locator
{
    virtual file_line source_location( srcloc sloc ) = 0;
};


class errors
{
public:

    errors( source_locator* locator, FILE* err );
    ~errors();
    
    void error( srcloc sloc, const char* format, ... ) PRINTF_FORMAT( 3, 4 );
    void warning( srcloc sloc, const char* format, ... ) PRINTF_FORMAT( 3, 4 );
    bool has_error();
    
    
private:

    void diagnostic( srcloc sloc, const char* kind, const char* format, va_list args );

    FILE* _err;
    source_locator* _locator;
    bool _has_error;

};




#endif



