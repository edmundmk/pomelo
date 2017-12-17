//
//  token.h
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#ifndef TOKEN_H
#define TOKEN_H


#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include "errors.h"


struct token;
class source;
template <> struct std::hash< token >;

typedef std::shared_ptr< source > source_ptr;


extern const token NULL_TOKEN;

struct token
{
    srcloc sloc;
    size_t text;
    size_t hash;

    explicit operator bool () const             { return text != 0; }
    bool operator == ( const token& t ) const   { return text == t.text; }
    bool operator != ( const token& t ) const   { return text == t.text; }
};

template <> struct std::hash< token >
{
    size_t operator () ( const token& t ) const { return t.hash; }
};


class source : public source_locator
{
public:
    
    explicit source( std::string_view path );
    source();
    
    void new_line( srcloc sloc );
    file_line source_location( srcloc sloc ) override;

    token new_token( srcloc sloc, std::string_view text );
    const char* text( const token& token ) const;
    

private:

    struct file
    {
        srcloc      sloc;
        std::string name;
        int         line;
    };
    
    std::string _path;
    std::vector< srcloc > _lines;
    std::vector< char > _text;
    std::unordered_map< std::string_view, size_t > _lookup;

};



#endif



