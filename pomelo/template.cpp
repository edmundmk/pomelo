//
//  parser template
//  pomelo
//
//  Created by Edmund Kapusniak on 31/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include "$(header)"
#include <assert.h>
#include <memory>
#include <algorithm>



/*
    Parsing tables.
*/

const int START_STATE      = $(start_state);

const int ERROR_ACTION     = $(error_action);
const int ACCEPT_ACTION    = $(accept_action);

const int TOKEN_COUNT      = $(token_count);
const int NTERM_COUNT      = $(nterm_count);
const int STATE_COUNT      = $(state_count);
const int RULE_COUNT       = $(rule_count);
const int CONFLICT_COUNT   = $(conflict_count);

const unsigned short ACTION[] =
{
$(action_table)
};

const unsigned short CONFLICT[] =
{
$(conflict_table)
};

const unsigned short GOTO[] =
{
$(goto_table)
};

const struct { unsigned short nterm; unsigned short length; const char* name; } RULE[] =
{
$(rule_table)
};



/*
    Get names of symbols.
*/

const char* $(class_name)_symbol_name( int kind )
{
    switch ( kind )
    {
    case 0: return "$";
    case $$(token_value): return "$$(raw_token_name)";
    case $(token_count): return "@start";
    case $$(nterm_value): return "$$(raw_nterm_name)";
    }
    return "";
}



/*
    A value on the parser stack.  Can hold value of a token or of a production.
*/

class $(class_name)::value
{
public:

    value()                                 : _state( -1 ), _kind( -2 ) {}
    explicit value( int s )                 : _state( s ), _kind( -2 ) {}

    value( int s, $$(ntype_type)&& v )      : _state( s ), _kind( $$(ntype_value) ) { new ( ($$(ntype_type)*)_storage ) $$(ntype_type)( std::move( v ) ); }
    
    value( value&& v )                      : _state( v._state ) { construct( std::move( v ) ); }
    value( const value& v )                 : _state( v._state ) { construct( v ); }
    value& operator = ( value&& v )         { if ( &v != this ) { destroy(); _state = v._state; construct( std::move( v ) ); } return *this; }
    value& operator = ( const value& v )    { if ( &v != this ) { destroy(); _state = v._state; construct( v ); } return *this; }
    
    ~value()                                { destroy(); }
    
    int state() const                       { return _state; }
    template < typename T > T& get() const  { return *(T*)_storage; }
    template < typename T > T&& move()      { return std::move( *(T*)_storage ); }
    
    
private:

    void construct( value&& v )
    {
        _kind = v._kind;
        switch ( _kind )
        {
        case $$(ntype_value): new ( ($$(ntype_type)*)_storage ) $$(ntype_type)( v.move< $$(ntype_type) >() ); break;
        }
    }
    
    void construct( const value& v )
    {
        _kind = v._kind;
        switch ( _kind )
        {
        case $$(ntype_value): new ( ($$(ntype_type)*)_storage ) $$(ntype_type)( v.get< $$(ntype_type) >() ); break;
        }
    }
    
    void destroy()
    {
        switch ( _kind )
        {
        case $$(ntype_value): ( ($$(ntype_type)*)_storage )->~$$(ntype_destroy)(); break;
        }
    }

    int _state;
    int _kind;
    std::aligned_storage_t
        <
        std::max
        ({
            (size_t)0
            , sizeof( $$(ntype_type) )
        })
        ,
        std::max
        ({
            (size_t)0
            , alignof( $$(ntype_type) )
        })
        >
    _storage[ 1 ];
};



/*
    Rules.
*/

$$(rule_type) $(class_name)::$$(rule_name)($$(rule_param)) { $$(rule_body) }



/*
    A piece of the parser stack.  The stack is a tree, with the leaves being
    the active stack tops.  This is how we implement generalized parsing.
*/

struct $(class_name)::piece
{
    int refcount;
    piece* prev;
    std::vector< value > values;
};



/*
    Implementation of the parser.
*/

?(user_value)$(class_name)::$(class_name)( const user_value& u )
?(user_value)    :   _anchor{ user_value(), -1, nullptr, &_anchor, &_anchor }
!(user_value)$(class_name)::$(class_name)()
!(user_value)    :   _anchor{ -1, nullptr, &_anchor, &_anchor }
{
    piece* p = new piece { 1, nullptr };
?(user_value)    stack* s = new stack { u, START_STATE, p, &_anchor, &_anchor };
!(user_value)    stack* s = new stack { START_STATE, p, &_anchor, &_anchor };
    _anchor.next = s;
    _anchor.prev = s;
}

$(class_name)::~$(class_name)()
{
    while ( _anchor.next != &_anchor )
    {
        delete_stack( _anchor.next );
    }
}

?(token_type)void $(class_name)::parse( int token, const token_type& v )
!(token_type)void $(class_name)::parse( int token )
{
    // Evaluate for each active parse stack.
    for ( stack* s = _anchor.next; s != &_anchor; s = s->next )
    {
        // Loop until this parse fails or we manage to shift the token.
        while ( true )
        {
            assert( s != &_anchor );

            // Look up action.
            int action = ACTION[ s->state * TOKEN_COUNT + token ];
            if ( action < STATE_COUNT )
            {
                // Shift and move to the state encoded in the action.
                printf( "SHIFT %s\n", $(class_name)_symbol_name( token ) );

                printf( "    %d :", s->state );
                for ( piece* p = s->head; p; p = p->prev )
                {
                    printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                }
                printf( "\n" );
                
?(token_type)                token_type tokval = v;
?(token_type)                s->head->values.push_back( value( s->state, std::move( tokval ) ) );
!(token_type)                s->head->values.push_back( value( s->state, std::nullptr_t() ) );
                s->state = action;

                printf( "--------\n" );
                for ( stack* s = _anchor.next; s != &_anchor; s = s->next )
                {
                    printf( "    %d :", s->state );
                    for ( piece* p = s->head; p; p = p->prev )
                    {
                        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                    }
                    printf( "\n" );
                }
                printf( "--------\n" );

                break;
            }
            else if ( action < STATE_COUNT + RULE_COUNT )
            {
                // Reduce using the rule.
                reduce( s, action - STATE_COUNT );

                // Continue around while loop until we do something other
                // than a reduction (a reduction does not consume the token).
            }
            else if ( action < STATE_COUNT + RULE_COUNT + CONFLICT_COUNT )
            {
                // Each active stack top is a piece with no referencers, We
                // create a unique stack piece for each action.  We continue
                // from the first stack we reduced with.
                stack* loop_stack = s->prev;
                stack* z = s->prev;
                
                // Get list of actions in the conflict.
                const auto* conflict = CONFLICT + action - STATE_COUNT - RULE_COUNT;
                int conflict_count = conflict[ 0 ];
                assert( conflict_count >= 2 );
                
                // Only the first action may be a shift
                int conflict_index = 1;
                if ( conflict[ conflict_index ] < STATE_COUNT )
                {
                    // Create a new stack.
                    z = split_stack( z, s );
                    printf( "===> SPLIT SHIFT %p %d", z, z->state );
                    for ( piece* p = z->head; p; p = p->prev )
                    {
                        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                    }
                    printf( "\n" );
                    
                    // Shift and move to the state encoded in the action.
                    int action = conflict[ conflict_index++ ];
?(token_type)                    token_type tokval = v;                    
?(token_type)                    z->head->values.push_back( value( z->state, std::move( tokval ) ) );
!(token_type)                    z->head->values.push_back( value( z->state, std::nullptr_t() ) );
                    z->state = action;

                    printf( "===> SPLIT AFTER %p %d", z, z->state );
                    for ( piece* p = z->head; p; p = p->prev )
                    {
                        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                    }
                    printf( "\n" );

                    // Ignore this stack until the next token.
                    loop_stack = z;
                }
                
                // Other actions must be reductions.
                while ( conflict_index < conflict_count )
                {
                    if ( conflict_index < conflict_count - 1 )
                    {
                        // Create a new stack.
                        z = split_stack( z, s );
                    }
                    else
                    {
                        // Continue using original stack.
                        assert( z->next == s );
                        z = s;

                        // Might have to create unique piece for this stack.
                        assert( z->head->refcount > 0 );
                        if ( z->head->refcount > 1 )
                        {
                            z->head = new piece { 1, z->head };
                        }
                    }

                    printf( "===> SPLIT REDUCE %p %d", z, z->state );
                    for ( piece* p = z->head; p; p = p->prev )
                    {
                        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                    }
                    printf( "\n" );
                
                    // Reduce using the rule.
                    int action = conflict[ conflict_index++ ];
                    assert( action >= STATE_COUNT && action < STATE_COUNT + RULE_COUNT );
                    reduce( z, action - STATE_COUNT );

                    printf( "===> SPLIT AFTER %p %d", z, z->state );
                    for ( piece* p = z->head; p; p = p->prev )
                    {
                        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                    }
                    printf( "\n" );

                    // If this is the first reduction, then we continue around
                    // the while loop until we do something other than a
                    // reduction (see below).  Otherwise this stack will be
                    // picked up on the next iteration of the main for loop,
                    // and the token consumed that way.
                }
               
                // Continue with the while loop using the first stack
                // resulting from a reduction.
                s = loop_stack->next;
            }
            else if ( action == ERROR_ACTION )
            {
                // If this is not the only stack, then destroy the stack.
                if ( s->next != &_anchor || s->prev != &_anchor )
                {
                    printf( "--DELETE %p--\n", s );
                    for ( stack* s = _anchor.next; s != &_anchor; s = s->next )
                    {
                        printf( "    %d :", s->state );
                        for ( piece* p = s->head; p; p = p->prev )
                        {
                            printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                        }
                        printf( "\n" );
                    }
                    printf( "--------\n" );    

                    delete_stack( ( s = s->prev )->next );

                    printf( "--AFTER--\n" );
                    for ( stack* s = _anchor.next; s != &_anchor; s = s->next )
                    {
                        printf( "    %d :", s->state );
                        for ( piece* p = s->head; p; p = p->prev )
                        {
                            printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
                        }
                        printf( "\n" );
                    }
                    printf( "--------\n" );    

                    break;
                }
                
                // Otherwise report the error.
?(user_value)?(token_type)                error( token, s->u, v );
?(user_value)!(token_type)                error( token, s->u );
!(user_value)?(token_type)                error( token, v );
!(user_value)!(token_type)                error( token );
                
                // TODO: error recovery.
                return;
            }
            else if ( action == ACCEPT_ACTION )
            {
                // Everything is fine, clean up by destroying the stack.
                delete_stack( ( s = s->prev )->next );
                break;
            }
        }
    }
}

void $(class_name)::reduce( stack* s, int rule )
{
    const auto& rule_info = RULE[ rule ];
    printf( "REDUCE %s\n", rule_info.name );

    // Find length of rule and ensure this stack has at least that many values.
    size_t length = rule_info.length;
    assert( s->head->refcount == 1 );
    while ( s->head->values.size() < length )
    {
        printf( "    %d :", s->state );
        for ( piece* p = s->head; p; p = p->prev )
        {
            printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
        }
        printf( "\n" );


        piece* prev = s->head->prev;
        assert( prev );
        
        if ( prev->refcount == 1 )
        {
            /*
                prev->prev <- prev <- s->head <- s

                prev->prev <- s->head <- s
            */

            // Move all values from this piece to the previous one.
            prev->values.reserve( prev->values.size() + s->head->values.size() );
            std::move
            (
                s->head->values.begin(),
                s->head->values.end(),
                std::back_inserter( prev->values )
            );
            
            // Move stack to from previous piece. to this piece.
            std::swap( prev->values, s->head->values );
            
            // Unlink and delete previous piece.
            s->head->prev = prev->prev;
            delete prev;
        }
        else
        {
            // Copy values from previous piece into this piece.
            size_t rq_count = length - s->head->values.size();
            size_t cp_count = std::min( prev->values.size(), rq_count );
            size_t index = prev->values.size() - cp_count;
            s->head->values.insert
            (
                s->head->values.begin(),
                prev->values.begin() + index,
                prev->values.end()
            );
            
            // Split previous piece.
            if ( index > 0 )
            {
                /*
                    prev->prev <- prev <- s->head <- s

                                        <- s->head <- s
                    prev->prev <- split
                                        <- prev
                */

                prev->refcount -= 1;
                assert( prev->refcount > 0 );

                // Create split piece and link it in.
                piece* split = new piece { 2, prev->prev };
                prev->prev = split;
                s->head->prev = split;
                
                // Move prev's stack to the split piece.
                std::swap( split->values, prev->values );
                
                // Move values after index from split to prev.
                prev->values.reserve( cp_count );
                std::move
                (
                    split->values.begin() + index,
                    split->values.end(),
                    std::back_inserter( prev->values )
                );
                split->values.erase
                (
                    split->values.begin() + index,
                    split->values.end()
                );
            }
            else
            {
                /*
                    prev->prev <- prev <- s->head <- s

                               <- s->head <- s
                    prev->prev
                               <- prev
                */
                
                prev->refcount -= 1;
                assert( prev->refcount > 0 );
                s->head->prev = prev->prev;
                if ( prev->prev )
                {
                    prev->prev->refcount += 1;
                }
            }
        }
    }

    printf( "    %d :", s->state );
    for ( piece* p = s->head; p; p = p->prev )
    {
        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
    }
    printf( "\n" );

    // Get pointer to values used to reduce.
    std::vector< value >& values = s->head->values;
    assert( values.size() >= length );
    size_t index = values.size() - length;
    value* p = values.data() + index;

    // Perform rule.
    switch ( rule )
    {
    case $$(rule_index): $$(rule_assign)$$(rule_name)($$(rule_args)) ); break;
    }
    
    // Remove excess elements.
    values.erase( values.begin() + index + 1, values.end() );
    
    // Find state we've returned to after reduction, and goto next one.
    int state = values[ index ].state();
    int gotos = GOTO[ state * NTERM_COUNT + rule_info.nterm ];
    assert( gotos < STATE_COUNT );
    s->state = gotos;

    printf( "--------\n" );
    for ( stack* s = _anchor.next; s != &_anchor; s = s->next )
    {
        printf( "    %d :", s->state );
        for ( piece* p = s->head; p; p = p->prev )
        {
            printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
        }
        printf( "\n" );
    }
    printf( "--------\n" );
}

?(user_value)?(token_type)void $(class_name)::error( int token, const user_value& u, const token_type& v )
?(user_value)!(token_type)void $(class_name)::error( int token, const user_value& u )
!(user_value)?(token_type)void $(class_name)::error( int token, const token_type& v )
!(user_value)!(token_type)void $(class_name)::error( int token )
{
    $(error_report)
}

?(user_value)$(class_name)::user_value $(class_name)::user_split( const user_value& u )
?(user_value){
?(user_value)    $(user_split)
?(user_value)}


$(class_name)::stack* $(class_name)::split_stack( stack* prev, stack* s )
{
    // Create new piece to be the head of the stack.
    piece* p = new piece { 1, s->head };
    p->prev->refcount += 1;

    // Create new stack.
?(user_value)    stack* split = new stack { user_split( s->u ), s->state, p, prev, prev->next };
!(user_value)    stack* split = new stack { s->state, p, prev, prev->next };
    split->prev->next = split;
    split->next->prev = split;

    return split;
}

void $(class_name)::delete_stack( stack* s )
{
    // Delete stack pieces.
    while ( s->head )
    {
        s->head->refcount -= 1;
        if ( s->head->refcount == 0 )
        {
            piece* prev = s->head->prev;
            delete s->head;
            s->head = prev;
        }
        else
        {
            break;
        }
    }
    
    // Unlink and then delete stack object itself.
    s->prev->next = s->next;
    s->next->prev = s->prev;
    delete s;
}

