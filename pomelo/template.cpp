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

const int START_STATE       = $(start_state);

const int STATE_COUNT       = $(state_count);
const int RULE_COUNT        = $(rule_count);
const int CONFLICT_COUNT    = $(conflict_count);

const short ACTION[] =
{
    $(action_table)
};

const short CONFLICT[] =
{
    $(conflict_table)
};

const short GOTO[] =
{
    $(goto_table)
};

const short LENGTH[] =
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

$(class_name)::$(class_name)()
    :   _anchor{ -1, nullptr, nullptr }
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

void $(class_name)::parse( int token, token_type&& v )
{
    // Evaluate for each active parse stack.
    for ( stack* next, stack = _anchor.next; next = stack->next, stack; stack = next )
    {
        piece* p = stack->piece;

        // Loop until this parse fails or we manage to shift the token.
        while ( true )
        {
            // Look up action.
            int action = ACTION[ stack->state * STATE_COUNT + token ];
            if ( action < STATE_COUNT )
            {
                // Shift and move to the state encoded in the action.
                p->values.push_back( value( v ) );
                break;
            }
            else if ( action < STATE_COUNT + RULE_COUNT )
            {
                // Reduce using the rule.
                reduce( action - STATE_COUNT );
                continue;
            }
            else if ( action < STATE_COUNT + RULE_COUNT + CONFLICT_COUNT )
            {
                // Perform all actions in the conflict.
                short* conflict = CONFLICT + action - STATE_COUNT - RULE_COUNT;
                
            
            
            }
            else if ( action == ERROR )
            {
                // TODO.
            }
            else if ( action == ACCEPT )
            {
                // TODO.
            }
        }
    }
}






