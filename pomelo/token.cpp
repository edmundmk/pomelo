//
//  token.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#include "token.h"
#include <assert.h>


extern const token NULL_TOKEN = { 0, 0, 0 };


source::source( std::string_view path )
    :   _path( path )
{
    _lines.push_back( 0 );
    _text.push_back( '\0' );
}

source::source()
{
}


void source::new_line( srcloc sloc )
{
    assert( ! _lines.empty() && _lines.back() < sloc );
    _lines.push_back( sloc );
}

file_line source::source_location( srcloc sloc )
{
    assert( ! _lines.empty() );

    auto l = std::upper_bound
    (
        _lines.begin(),
        _lines.end(),
        sloc
    );
    l--;
    
    int line = (int)( l - _lines.begin() ) + 1;
    int column = (int)( sloc - *l );
    
    return { _path.c_str(), line, column };
}



token source::new_token( srcloc sloc, std::string_view text )
{
    size_t toff = 0;
    auto i = _lookup.find( text );
    if ( i != _lookup.end() )
    {
        // Token text already in buffer.
        toff = i->second;
    }
    else
    {
        // Add text to buffer.
        toff = _text.size();
        size_t old_capacity = _text.capacity();
        _text.insert( _text.end(), text.begin(), text.end() );
        _text.push_back( '\0' );
        
        // Check if we have to rebuild the index.
        if ( _text.capacity() != old_capacity )
        {
            std::unordered_map< std::string_view, size_t > new_lookup;
            for ( const auto& s : _lookup )
            {
                new_lookup.emplace( _text.data() + s.second, s.second );
            }
            std::swap( _lookup, new_lookup );
        }
        
        _lookup.emplace( _text.data() + toff, toff );
    }

    // Construct a token.
    token token;
    token.sloc = sloc;
    token.text = toff;
    token.hash = std::hash< std::string_view >()( text );

    return token;
}

const char* source::text( const token& token ) const
{
    return _text.data() + token.text;
}



