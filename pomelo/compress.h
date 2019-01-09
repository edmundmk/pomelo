//
//  compress.h
//
//  Created by Edmund Kapusniak on 09/01/2019.
//  Copyright © 2019 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#ifndef COMPRESS_H
#define COMPRESS_H


#include <vector>
#include <memory>


struct compressed_table;
typedef std::shared_ptr< compressed_table > compressed_table_ptr;


/*
    Given a two-dimensional array of integers, produce a compressed table
    using row displacements.  Each rows is mapped into compress at a location
    where its entries do not overlap any entries already in compress.

    Lookup is done using:

        int rowdisp = displace[ row ];
        int entry_val = compress[ rowdisp + column ];
        int entry_row = comprows[ rowdisp + column ];
        return entry_row == row ? entry_val : error;

*/

struct compressed_table
{
    int cols; // number of columns in original table.
    int rows; // number of rows in original table.

    int error; // error value, expected to be the maximum value in table.

    std::vector< int > displace; // row displacements for each row.
    std::vector< int > compress; // compressed non-error data.
    std::vector< int > comprows; // which row each data entry was plucked from.
};


compressed_table_ptr compress( int cols, int rows, int error, const std::vector< int >& table );



#endif

