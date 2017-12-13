//
//  token.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 11/12/2017.
//  Copyright © 2017 Edmund Kapusniak. All rights reserved.
//

#include "token.h"



source::source( std::string_view path )
    :   _path( path )
{
    _text.push_back( '\0' );
}

source::source()
{
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



