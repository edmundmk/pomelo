//
//  parser template
//  pomelo
//
//  Created by Edmund Kapusniak on 31/01/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include "$(header)"
#include <memory>


const int ERROR  = -1;
const int ACCEPT = -2;


/*
    Parsing tables.
*/

const int START_STATE      = $(start_state);

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

const unsigned short LENGTH[] =
{
    $(rule_length)
};



/*
    A value on the parser stack.  Can hold value of a token or of a production.
*/

struct $(class_name)::value
{
    value( int s, const token_type& v ) : state( s ), kind( -1 ) { new ( (token_type*)storage ) token_type( v ); }
    value( int s, $(nterm_type)&& v ) : state( s ), kind( $(nterm_value) ) { new ( ($(nterm_type)*)storage ) $(nterm_type)( std::move( v ) ); }
    
    value( value&& v ) : state( v.state ), kind( v.kind )
    {
        switch ( kind )
        {
        default: new ( (token_type*)storage ) token_type( std::move( *( (token_type*)v.storage ) ) ); break;
        case $(nterm_value): new ( ($(nterm_type)*)storage ) $(nterm_type)( std::move( *( ($(nterm_type)*)v.storage ) ) ); break;
        }
    }
    
    ~value()
    {
        switch ( kind )
        {
        default: ( (token_type*)storage )->~token_type() ); break;
        case $(nterm_value): ( ($(nterm_type)*)storage )->~$(nterm_type)(); break;
        }
    }

    int state;
    int kind;
    std::aligned_storage_t
        <
              sizeof( token_type )
            + sizeof( $(nterm_type) )
        ,
        std::max
        ({
              alignof( token_type )
            + alignof( $(nterm_type) )
        })
        >
    storage[ 1 ];
};



/*
    Rules.
*/

$(rule_type) $(class_name)::$(rule_name)($(rule_param)) { $(rule_body) }



/*
    A piece of the parser stack.  The stack is a tree, with the leaves being
    the active stack tops.  This is how we implement generalized parsing.
*/

struct $(class_name)::piece
{
    int refcount;
    std::vector< value > values;
}



/*
    The top of an active parse stack.  They are in a doubly-linked list.
*/

struct $(class_name)::stack
{
    int state;
    piece* piece;
    stack* prev;
    stack* next;
};



/*
    Implementation of the parser.
*/

$(class_name)::$(class_name)( const user_value& u )
    :   u( u )
    ,   _anchor{ -1, nullptr, nullptr }
{
    new_stack( &_anchor, START_STATE );
}

$(class_name)::~$(class_name)()
{
    while ( _anchor.next )
    {
        delete_stack( _anchor.next );
    }
}

void $(class_name)::parse( int token, const token_type& v )
{
    // Evaluate for each active parse stack.
    for ( stack* s = _anchor.next; s; s = s->next )
    {
        assert( s->state >= 0 );
    
        // Loop until this parse fails or we manage to shift the token.
        while ( true )
        {
            // Look up action.
            int action = ACTION[ s->state * STATE_COUNT + token ];
            if ( action < STATE_COUNT )
            {
                // Shift and move to the state encoded in the action.
                s->piece->values.push_back( value( v ) );
                s->state = action;
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
                // We reuse the current stack for the last action in the
                // conflict.  We continue from the first non-shift stack.
                stack* prev = s->prev;
                stack* z = prev;

                // Get list of actions in the conflict.
                const auto* conflict = CONFLICT + action - STATE_COUNT - RULE_COUNT;
                int size = conflict[ 0 ];
                assert( size >= 2 );
                
                // Only the first action may be a shift
                if ( conflict[ 1 ] < STATE_COUNT )
                {
                    // Create a new stack.
                    z = new_stack( z, s->state );
                    
                    // Shift and move to the state encoded in the action.
                    int action = conflict[ 1 ];
                    z->piece->values.push_back( value( v ) );
                    z->state = action;
                    
                    // Ignore this stack until the next token.
                    prev = z;
                }
                
                // Other actions must be reductions.
                for ( int i = 1; i < size; ++i )
                {
                    // Create stack for this reduction, or reuse original.
                    if ( i < size - 1 )
                        z = new_stack( z, s->state );
                    else
                        z = s;
                
                    // Reduce using the rule.
                    int action = conflict[ i ];
                    assert( action >= STATE_COUNT && action < STATE_COUNT + RULE_COUNT );
                    reduce( z, action - STATE_COUNT );
                    
                    // If this is the first reduction, then we continue around
                    // the while loop until we do something other than a
                    // reduction (see below).  Otherwise this stack will be
                    // picked up on the next iteration of the main for loop,
                    // and the token consumed that way.
                }
                
                // Continue with the while loop using the first stack
                // resulting from a reduction.
                s = prev;
            }
            else if ( action == ERROR )
            {
                // If this is not the only stack, then destroy the stack.
                if ( s->next == &_anchor && s->prev == &_anchor )
                {
                    delete_stack( ( s = s->prev )->next );
                    break;
                }
                
                // Otherwise report the error.
                
                
            }
            else if ( action == ACCEPT )
            {
                // Everything is fine, clean up by destroying the stack.
                delete_stack( ( s = s->prev )->next );
            }
        }
    }
}






