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
    $$(token_name) = $$(token_value),
};

enum
{
    $$(nterm_name) = $$(nterm_value),
};


const char* $(class_name)_symbol_name( int kind );


class $(class_name)
{
public:

?(user_value)    typedef $(user_value) user_value;
?(token_type)    typedef $(token_type) token_type;
    
?(user_value)    explicit $(class_name)( const user_value& u );
!(user_value)    $(class_name)();
    ~$(class_name)();
    
?(token_type)    void parse( int token, const token_type& v );
!(token_type)    void parse( int token );


private:

    struct empty;
    class  value;
    struct piece;
    
    struct stack
    {
        int state;
        piece* head;
        stack* prev;
        stack* next;
    };
    
    $$(rule_type) $$(rule_name)($$(rule_param));
    
    void reduce( stack* s, int rule );
?(token_type)    void error( int token, const token_type& v );
!(token_type)    void error( int token );
    
    stack* new_stack( stack* list, piece* prev, int state );
    void delete_stack( stack* s );
    
?(user_value)    user_value u;
    stack _anchor;

};

