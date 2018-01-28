//
//  template.h
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <vector>

class parser
{
public:

    typedef int token_value;

    enum token_kind
    {
    };
    
    enum nterm_kind
    {
    };

    parser();
    ~parser();
    
    void parse( token_kind token, const token_value& value );
    
    
private:

    struct action
    {
        int symbol;
        int action;
    };

    struct stack_entry
    {
        int state;
        std::aligned_storage< 10 > storage;
    };
    
    struct stack_piece
    {
        int refcount;
        stack_piece* previous;
        std::vector< stack_entry > stack;
    };

    


    std::vector< stack_piece* > _heads;

};
















#endif

