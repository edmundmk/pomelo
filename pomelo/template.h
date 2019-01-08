//
//  parser template
//  pomelo
//
//  Created by Edmund Kapusniak on 28/01/2018.
//  Copyright © 2018 Edmund Kapusniak.
//
//  Licensed under the MIT License. See LICENSE file in the project root for
//  full license information. 
//

#ifndef $(include_guard)
#define $(include_guard)

#include <vector>

$(include_header)

enum
{
    $$(token_name) = $$(token_value),
};

enum
{
    $$(nterm_name) = $$(nterm_value),
};


class $(class_name)
{
public:

    static const char* symbol_name( int kind );

?(user_value)    typedef $(user_value) user_value;
?(token_type)    typedef $(token_type) token_type;
    
?(user_value)    explicit $(class_name)( const user_value& u );
!(user_value)    $(class_name)();
    ~$(class_name)();
    
?(token_type)    void parse( int token, const token_type& tokval );
!(token_type)    void parse( int token );


private:

    class value;

    struct rule_info
    {
        unsigned short nterm;
        unsigned short length   : 15;
        unsigned short merges   : 1;
    };

    struct piece
    {
        int refcount;
        piece* prev;
        std::vector< value > values;
    };
    
    struct stack
    {
?(user_value)        user_value u;
        int state;
        piece* head;
        stack* prev;
        stack* next;
    };

    static const int START_STATE;
    static const int TOKEN_COUNT;
    static const int NTERM_COUNT;
    static const int STATE_COUNT;
    static const int RULE_COUNT;
    static const int CONFLICT_COUNT;
    static const int ERROR_ACTION;
    static const int ACCEPT_ACTION;

    static const unsigned short ACTION[];
    static const unsigned short CONFLICT[];
    static const unsigned short GOTO[];
    static const rule_info RULE[];

    $$(rule_type) $$(rule_name)($$(rule_param));
    $$(merge_type) $$(merge_name)( const user_value& u, $$(merge_type)&& a, user_value&& v, $$(merge_type)&& b );
    
    int lookup_action( int state, int token );
    void reduce( stack* s, int token, int rule );
    void reduce_rule( stack* s, int rule, const rule_info& rinfo );
?(user_value)?(token_type)    void error( const user_value& u, int token, const token_type& tokval );
?(user_value)!(token_type)    void error( const user_value& u, int token );
!(user_value)?(token_type)    void error( int token, const token_type& tokval );
!(user_value)!(token_type)    void error( int token );
?(user_value)    user_value user_split( const user_value& u );
    stack* split_stack( stack* prev, stack* s );
    void delete_stack( stack* s );
    
#ifdef POMELO_TRACE
    void dump_stack( stack* s );
    void dump_stacks();
#endif

    stack _anchor;

};

#endif
