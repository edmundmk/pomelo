//
//  options.h
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//


#ifndef OPTIONS_H
#define OPTIONS_H


#include <string>
#include <vector>


struct options
{
    options();
    ~options();
    
    bool parse( int argc, const char* argv[] );
    
    std::string source;
    std::string output_c;
    std::string output_h;
    bool syntax;
    bool dump;
    bool graph;
    bool rgoto;
    bool actions;
    bool conflicts;

};


#endif

