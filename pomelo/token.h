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
#include "diagnostics.h"


class token;
class token_source;
template <> struct std::hash< token >;

typedef std::shared_ptr< token_source > token_source_ptr;


class token
{
public:

    token();
    ~token();
    
    explicit operator bool () const             { return _text != 0; }
    bool operator == ( const token& t ) const   { return _text == t._text; }
    bool operator != ( const token& t ) const   { return _text == t._text; }
    

private:

    friend struct std::hash< token >;

    srcloc _sloc;
    size_t _text;
    size_t _hash;

};


template <> struct std::hash< token >
{
    size_t operator () ( const token& t ) const { return t._hash; }
};


class token_source
{
public:
    
    explicit token_source( std::string_view path );
    ~token_source();
    
    token new_token( srcloc sloc, std::string_view text );
    std::string_view text( const token& token ) const;
    

private:

    std::string _path;
    std::vector< char > _text;

};



#endif



