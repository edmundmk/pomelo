//
//  options.h
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
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
    bool syntax;
    bool graph;
    bool graph_rgoto;
    bool actions;

};


#endif

