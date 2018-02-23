//
//  parser template
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//

#ifndef $(include_guard)
#define $(include_guard)

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

    class value;
    struct piece;
    
    struct stack
    {
?(user_value)        user_value u;
        int state;
        piece* head;
        stack* prev;
        stack* next;
    };
    
    $$(rule_type) $$(rule_name)($$(rule_param));
    
    void reduce( stack* s, int rule );
?(user_value)?(token_type)    void error( int token, const user_value& u, const token_type& v );
?(user_value)!(token_type)    void error( int token, const user_value& u );
!(user_value)?(token_type)    void error( int token, const token_type& v );
!(user_value)!(token_type)    void error( int token );
?(user_value)    user_value user_split( const user_value& u );
    stack* split_stack( stack* prev, stack* s );
    void delete_stack( stack* s );
    
    stack _anchor;

};

#endif
