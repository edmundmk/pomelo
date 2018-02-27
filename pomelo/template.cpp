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

const int $(class_name)::START_STATE      = $(start_state);
const int $(class_name)::TOKEN_COUNT      = $(token_count);
const int $(class_name)::NTERM_COUNT      = $(nterm_count);
const int $(class_name)::STATE_COUNT      = $(state_count);
const int $(class_name)::RULE_COUNT       = $(rule_count);
const int $(class_name)::CONFLICT_COUNT   = $(conflict_count);
const int $(class_name)::ERROR_ACTION     = $(error_action);
const int $(class_name)::ACCEPT_ACTION    = $(accept_action);

const unsigned short $(class_name)::ACTION[] =
{
$(action_table)
};

const unsigned short $(class_name)::CONFLICT[] =
{
$(conflict_table)
};

const unsigned short $(class_name)::GOTO[] =
{
$(goto_table)
};

const $(class_name)::rule_info $(class_name)::RULE[] =
{
$(rule_table)
};



/*
    Get names of symbols.
*/

const char* $(class_name)::symbol_name( int kind )
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
    Merges.
*/

$$(merge_type) $(class_name)::$$(merge_name)( const user_value& u, $$(merge_type)&& a, user_value&& v, $$(merge_type)&& b ) { $$(merge_body) }


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
            int action = lookup_action( s->state, token );
            if ( action < STATE_COUNT )
            {
                // Shift and move to the state encoded in the action.
                printf( "SHIFT %s\n", symbol_name( token ) );
                dump_stack( s );
                
?(token_type)                token_type tokval = v;
?(token_type)                s->head->values.push_back( value( s->state, std::move( tokval ) ) );
!(token_type)                s->head->values.push_back( value( s->state, std::nullptr_t() ) );
                s->state = action;

                dump_stacks();
                break;
            }
            else if ( action < STATE_COUNT + RULE_COUNT )
            {
                // Reduce using the rule.
                reduce( s, token, action - STATE_COUNT );

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
                const unsigned short* conflict = CONFLICT + action - STATE_COUNT - RULE_COUNT;
                int conflict_count = conflict[ 0 ];
                assert( conflict_count >= 2 );
                
                // Only the first action may be a shift
                int conflict_index = 1;
                if ( conflict[ conflict_index ] < STATE_COUNT )
                {
                    // Create a new stack.
                    z = split_stack( z, s );
                    printf( "===> SPLIT SHIFT %p %d", z, z->state );
                    dump_stack( z );
                    
                    // Shift and move to the state encoded in the action.
                    int action = conflict[ conflict_index++ ];
?(token_type)                    token_type tokval = v;                    
?(token_type)                    z->head->values.push_back( value( z->state, std::move( tokval ) ) );
!(token_type)                    z->head->values.push_back( value( z->state, std::nullptr_t() ) );
                    z->state = action;

                    printf( "===> SPLIT AFTER %p %d", z, z->state );
                    dump_stack( z );

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
                    dump_stack( z );
                
                    // Reduce using the rule.
                    int action = conflict[ conflict_index++ ];
                    assert( action >= STATE_COUNT && action < STATE_COUNT + RULE_COUNT );
                    reduce( z, token, action - STATE_COUNT );

                    printf( "===> SPLIT AFTER %p %d", z, z->state );
                    dump_stack( z );

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
                    dump_stacks();

                    delete_stack( ( s = s->prev )->next );

                    printf( "--AFTER--\n" );
                    dump_stacks();

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


int $(class_name)::lookup_action( int state, int token )
{
    return ACTION[ state * TOKEN_COUNT + token ];
}

void $(class_name)::reduce( stack* s, int token, int rule )
{
    // Perform reduction.
    const rule_info& rinfo = RULE[ rule ];
    reduce_rule( s, rule, rinfo );

    // Unless this reduction could merge stacks, return.
    if ( ! rinfo.merges || s->head->values.size() != 1 || s->next == &_anchor )
    {
        return;
    }

    // Check other stacks for a reduction to this same nonterminal.  Stacks
    // earlier in the list will already have been reduced.
    for ( stack* z = s->next; z != &_anchor; z = z->next )
    {
        // Track fake state for this parse stack as we 'reduce' it.
        int state = z->state;
        piece* head = z->head;
        size_t size = z->head->values.size();

        // Simulate reductions until we hit a mergeable state.
        bool merge = false;
        while ( true )
        {
            int action = lookup_action( state, token );
            if ( action < STATE_COUNT || action >= STATE_COUNT + RULE_COUNT )
            {
                // Not a reduction.  If your grammar is insane enough that a
                // conflicting reduction can merge then I don't want to know.
                break;
            }

            // Simulate reduction.
            int zrule = action - STATE_COUNT;
            const rule_info& zrinfo = RULE[ zrule ];

            // Pop symbols off 'stack'.
            size_t length = zrinfo.length;
            if ( length > 0 )
            {
                while ( length > 0 )
                {
                    if ( size == 0 )
                    {
                        head = head->prev;
                        size = z->head->values.size();
                    }
                    size_t count = std::min( size, length );
                    size -= count;
                    length -= count;
                }

                // This will be wrong if the grammar contains any rules
                // which entirely composed of erasable rules.
                state = head->values.at( size ).state();
            }

            // Move to state.
            int goto_state = GOTO[ state * NTERM_COUNT + zrinfo.nterm ];
            assert( goto_state < STATE_COUNT );
            state = goto_state;

            // Check if we've reduced to the target symbol, and this is
            // the only difference with the target stack.
            if ( state == s->state && zrinfo.nterm == rinfo.nterm
                    && size == 0 && head->prev == s->head->prev )
            {
                // Merges.
                merge = true;
                break;
            }

            // Push nonterminal onto 'stack'.
            size += 1;
        }

        // Unless we can merge, check next stack.
        if ( ! merge )
        {
            continue;
        }

        printf( "====> MERGE\n" );
        dump_stacks();

        // This stack merges.  Actually perform reductions.
        while ( true )
        {
            // Look up action, which must be a reduction.
            int action = lookup_action( state, token );
            assert( action >= STATE_COUNT && action < STATE_COUNT + RULE_COUNT );

            // Perform reduction.
            int zrule = action - STATE_COUNT;
            const rule_info& zrinfo = RULE[ zrule ];
            reduce_rule( z, zrule, zrinfo );

            // Check for arrival at mergable state.
            if ( state == s->state && zrinfo.nterm == rinfo.nterm
                    && z->head->values.size() == 1 && z->head->prev == s->head->prev )
            {
                break;
            }
        }

        // Double check that I'm not crazy.
        assert( s->head->prev == z->head->prev );
        assert( s->head->values.size() == 1 );
        assert( z->head->values.size() == 1 );

        dump_stacks();

        // Perform merge.
        value& a = s->head->values[ 0 ];
        value& b = z->head->values[ 0 ];
        switch ( rinfo.nterm )
        {
        case $$(merge_index): a = value( a.state(), $$(merge_name)( s->u, a.move< $$(merge_type) >(), std::move( z->u ), b.move< $$(merge_type) >() ) ); break;
        }

        // Delete stack.
        z->head->values.erase( z->head->values.begin() );
        delete_stack( z );

        dump_stacks();
    }
}

void $(class_name)::reduce_rule( stack* s, int rule, const rule_info& rinfo )
{
    printf( "REDUCE %s\n", symbol_name( rinfo.nterm ) );

    // Find length of rule and ensure this stack has at least that many values.
    size_t length = rinfo.length;
    assert( s->head->refcount == 1 );
    while ( s->head->values.size() < length )
    {
        dump_stack( s );

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

    dump_stack( s );

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
    int goto_state = GOTO[ state * NTERM_COUNT + rinfo.nterm ];
    assert( goto_state < STATE_COUNT );
    s->state = goto_state;

    dump_stacks();
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


/*
    Debugging.
*/

void $(class_name)::dump_stacks()
{
    printf( "--------\n" );
    for ( stack* s = _anchor.next; s != &_anchor; s = s->next )
    {
        dump_stack( s );
    }
    printf( "--------\n" );
}

void $(class_name)::dump_stack( stack* s )
{
    printf( "    %p->%d :", s, s->state );
    for ( piece* p = s->head; p; p = p->prev )
    {
        printf( " -> %p/%d/%zu", p, p->refcount, p->values.size() );
    }
    printf( "\n" );
}



