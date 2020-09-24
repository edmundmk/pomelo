//
//  compress.cpp
//
//  Created by Edmund Kapusniak on 09/01/2019.
//  Copyright © 2019 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information.
//


#include "compress.h"
#include <assert.h>
#include <algorithm>


struct table_row
{
    int row;
    int index;
    int value_count;
};


static bool conflicting( const std::vector< int >& table, int t_index, const compressed_table_ptr& c, int c_index )
{
    assert( (unsigned)c_index <= c->compress.size() );
    int count = std::min( c->cols, (int)( c->compress.size() - c_index ) );
    for ( int col = 0; col < count; ++col )
    {
        bool table_has_value = table[ t_index + col ] != c->error;
        bool compress_has_value = c->compress[ c_index + col ] != c->error;
        if ( table_has_value && compress_has_value )
        {
            return true;
        }
    }
    return false;
}


compressed_table_ptr compress( int cols, int rows, int error, const std::vector< int >& table )
{
    assert( table.size() == (unsigned)( cols * rows ) );

    // Create result.
    compressed_table_ptr c = std::make_shared< compressed_table >();
    c->cols = cols;
    c->rows = rows;
    c->error = error;

    // Count number of non-error entries in each row of the table.
    std::vector< table_row > table_rows;
    table_rows.reserve( rows );
    for ( int row = 0; row < rows; ++row )
    {
        table_row tr = { row, row * cols, 0 };
        for ( int col = 0; col < cols; ++col )
        {
            assert( table[ tr.index + col ] <= error );
            if ( table[ tr.index + col ] != error )
            {
                tr.value_count += 1;
            }
        }
        table_rows.push_back( tr );
    }

    // Sort original table rows descending by number of non-error-entries.
    std::sort
    (
        table_rows.begin(),
        table_rows.end(),
        []( const table_row& a, const table_row& b )
        {
            return a.value_count > b.value_count;
        }
    );

    // Construct compress tables and displacements.
    c->displace.resize( rows );
    for ( int sorted_row = 0; sorted_row < rows; ++sorted_row )
    {
        table_row tr = table_rows[ sorted_row ];

        // Find first position in compressed table where there is no conflict.
        int disp = 0;
        for ( ; (unsigned)disp < c->compress.size(); ++disp )
        {
            if ( ! conflicting( table, tr.index, c, disp ) )
            {
                break;
            }
        }

        // Place row here.
        c->displace[ tr.row ] = disp;
        int col = 0;
        for ( ; col < cols && (unsigned)( disp + col ) < c->compress.size(); ++col )
        {
            int value = table[ tr.index + col ];
            if ( value != error )
            {
                assert( c->compress[ disp + col ] == error );
                assert( c->comprows[ disp + col ] == rows );
                c->compress[ disp + col ] = value;
                c->comprows[ disp + col ] = tr.row;
            }
        }
        for ( ; col < cols; ++col )
        {
            int value = table[ tr.index + col ];
            c->compress.push_back( value );
            c->comprows.push_back( value != error ? tr.row : rows );
        }

    }

    for ( int row = 0; row < rows; ++row )
    {
        for ( int col = 0; col < cols; ++col )
        {
            int c_index = c->displace[ row ] + col;
            int c_value = c->comprows[ c_index ] == row ? c->compress[ c_index ] : c->error;
            int t_value = table[ row * cols + col ];
            assert( c_value == t_value );
        }
    }

    return c;
}


