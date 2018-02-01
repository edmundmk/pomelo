//
//  parser template
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include <vector>

$(include)

enum
{
    $(tokens),
};

enum
{
    $(nterms),
};

class $(class_name)
{
public:

    typedef $(user_value) user_value;
    typedef $(token_type) token_type;
    
    explicit $(class_name)( const user_value& u );
    ~$(class_name)();
    
    void parse( int token, const token_type& v );


private:

    struct value;
    struct piece;
    struct stack;
    
    $(rule_type) $(rule_name)($(rule_param));
    
    void reduce( int rule_index );
    
    stack* new_stack( stack* prev, int state );
    void delete_stack( stack* s );
    
    user_value u;
    stack _anchor;

};



