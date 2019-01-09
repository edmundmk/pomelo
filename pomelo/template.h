//
//  pomelo parser interface
//
//  Copyright 2018 Edmund Kapusniak
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//

#ifndef $(include_guard)
#define $(include_guard)

#include <vector>

$(include_header)

enum
{
    $(token_prefix)EOI = 0,
    $(token_prefix)$$(token_name) = $$(token_value),
};

enum
{
    $(nterm_prefix)$$(nterm_upper) = $$(nterm_value),
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
    static const int ACCEPT_ACTION;
    static const int ERROR_ACTION;

    static const unsigned short ACTION_DISPLACEMENT[];
    static const unsigned short ACTION_VALUE_TABLE[];
    static const unsigned short ACTION_ROW_TABLE[];
    static const unsigned short GOTO_DISPLACEMENT[];
    static const unsigned short GOTO_VALUE_TABLE[];
    static const unsigned short GOTO_ROW_TABLE[];
    static const unsigned short CONFLICT[];
    static const rule_info RULE[];

    $$(rule_type) $$(rule_name)($$(rule_param));
    $$(merge_type) $$(merge_name)( const user_value& u, $$(merge_type)&& a, user_value&& v, $$(merge_type)&& b );
    
    int lookup_action( int state, int token );
    int lookup_goto( int state, int nterm );
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
