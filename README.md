# pomelo

pomelo is a LALR(1) parser generator with support for GLR parsing.  It is
similar to bison or lemon, but produces parsers implemented in modern C++,
taking advantage of compiler features such as move semantics.


## Building pomelo

pomelo is built using meson, e.g.

    meson build; cd build; ninja

The build scripts assume a Unix environment, and the `xxd` tool is used to
embed the parser template into the program.


## Running pomelo

    pomelo [options] -h <output header> -c <output source> <syntax file>

Options:

  * `--syntax` : Writes a syntax file to stdout after parsing, generated
    from the internal syntax representation.  Used to test the syntax parser 
    frontend.

  * `--dump` : Writes a detailed list of all states and transitions in the
    generated LALR(1) automata to stdout.  Useful for debugging complex
    grammars and understanding conflicts.

  * `--graph` : Writes a digraph description of the LALR(1) automata in `dot`
    format, which can be used to generate a visual representation of the
    automata.  More useful if a problem can be localised to small example
    grammar, as otherwise the graph is likely to be extremely large.

  * `--rgoto` : Adds additional arrows to the `--graph` which link the final
    transition in a production to the transitions that shift the reduced
    production onto the parser stack.  This corresponds to the `includes`
    relation in LALR(1) construction, and can be useful for debugging the
    parser generator.

  * `--actions` : Dump a low-level representation of the main parser table
    to stdout.

  * `--conflicts` : Print warnings about expected conflicts, and conflicts
    resolved by token precedence to stderr.  Normally, pomelo only reports
    unexpected and unresolved conflicts.


## Generated Parser

pomelo generates a parser class, including all actions from the syntax file.

To use the parser, construct an instance, passing in the user value for the
main parse stack.  The user value is copied into the parse stack.  It is
typically a smart pointer to the object built or modified by the parser, or a
pointer to an object that provides methods called by the actions.

    explicit parser( const user_value& u );

Parsing then proceeds one token at a time.  The generated header contains an
enumeration assigning token numbers to each terminal in the grammar.  Call the
`parse` method once for each token, passing in both the token number and the
token's value.

    void parse( int token, const token_type& tokval );

The special `EOI` token is declared implicitly, representing the end of input.
It always has a token number of 0.  Call the `parse` method again with this
token to complete a parse.


## Syntax Files

### Productions

Syntax files consist of a list of nonterminal productions.  Each production
consists of its name, followed by a list of rules enclosed in square brackets.

    nonterminal_name
    [
        A B C .
        D E F .
    ]

Nonterminal (production) names are lower-case.  Terminal (token) names are
upper-case.  All rules for a particular production must be specified at the 
same time.

The first nonterminal in a syntax file is the root production.


### Rules

Each rule consists of a list of symbols.  Each symbol can either be a terminal
or a nonterminal.  Each rule is terminated by a period.

        PP contents DOT qual_name QQ .

An epsilon rule indicating a production with no symbols can be specified using
a period on its own.

        .

Each rule can have a C++ action associated with it, which is executed when
the rule is reduced.  C++ code is placed after the period at the end of the
rule, and is enclosed in curly braces.

        PP contents DOT qual_name QQ .
            { return reduced_rule(); }

The values of the symbols comprising the rule can be provides as arguments to
the action function by giving them a name using parentheses.

        PP contents(c) DOT(op) qual_name(name) QQ .
            { return build_node( c, op, name); }

Symbol values are passed by rvalue reference into the action function, as they
are consumed by the action to create the resulting nonterminal value.

There is an additional parameter, `u`, which is a reference to the user value
for the parse stack.

Action functions must return a value for the resulting nonterminal value.  The
type of a nonterminal is specified in curly braces after the nonterminal name.

    pp_qq_opt { node_ptr }
    [
        .
            { return node_ptr(); }
        PP contents(c) DOT(op) qual_name(name) QQ .
            { return build_node( c, op, name); }
    ]

Nonterminal values are placed on the parse stack.  They are usually moved -
from the result of an action function into the parse stack, and then out of
the parse stack into the action function that consumes them.  However, when
parse stacks are split they may be copied, as each potential parse gets its
own copy of each value.


### Precedence

pomelo constructs LALR(1) parsers and therefore the generated automaton can
contain shift/reduce or reduce/reduce conflicts.  In most cases the author of
a grammar would like the generated parser to pick one action (either shift or
reduce) for each conflict.

Like other parser generators, pomelo uses token precedence to resolve
conflicts.

Precendence and associativity is assigned to terminal symbols using the
`%left`, `%right`, and `%nonassoc` directives.

    %nonassoc NO_ELSE .
    %nonassoc ELSE .

    %right ASSIGN .
    %left ADD SUB .
    %left MUL DIV MOD .

Tokens appearing in earlier directives have lower precedence (i.e. when used
as operators they bind less tightly).  Tokens appearing in later directives
have higher precedence (i.e. they bind more tightly).  Tokens which appear in
the same directive have the same precedence.

These directives also provide a way to declare terminal symbols which do not
appear in any grammar rule.

The default precedence of a rule is the same as the precedence of the first
terminal symbol in the rule.  This can be overridden by specifying a token
in square brackets between the period and the rule's action.  The precedence
of the rule is then the same as the precedence of this token.

        IF LPN expr RPN stmt . [NO_ELSE] { return ifstmt(); }

When the parser generator encounters a conflict, it first attempts to resolve
it using the precedence rules.  These follow the same strategy as the `lemon`
parser generator.

For a shift/reduce conflict:

  * If either the token to shift or the rule to reduce lacks precedence, the
    conflict is unresolved.
  * If the token has higher precedence than the rule, shift.
  * If the rule has higher precedence than the token, reduce.
  * Otherwise they have the same precedence, so consider the associativity of
    the token.
  * If the token is right-associative, shift.
  * If the token is left-associative, reduce.
  * If the token is non-associative, the conflict is unresolved.


For a reduce/reduce conflict:

  * If either rule lacks precedence, the conflict is unresolved.
  * If both rules have the same precedence, the conflict is unresolved.
  * Otherwise, reduce the rule with the higher precedence.


Using these rules most conflicts in programming language grammars can be
resolved.


### Expected Conflicts

Many real grammars are not LALR(1) and will contain parse conflicts.  By
default, pomelo reports an error (and returns a failure code) when it
encounters unresolved conflicts in a grammar.

When writing a grammar, especially for an LR parser, conflicts are not always
obvious.  It is easy to edit a grammar and introduce ambiguity where none was
intended.  However, some ambiguities are real features of the language and
should be accepted by the parser generator - they will trigger a split in the
parser stack at runtime and parsing will continue using a GLR algorithm.

Therefore, a grammar author must mark expected unresolved conflicts using
the conflict marker `!` where they appear in the grammar.  This reduces the
chance that an unanticipated conflict will slip through.

To mark a shift as conflicting, place a conflict marker before the token that
will be shifted where it appears in a rule.

To mark a rule as conflicting, place a conflict marker at the end of the rule,
before the period.

    conflict_marker_example
    [
        context ! SHIFT_TOKEN TRAILING .
        reduce_rule TOKEN symbol ! .
    ]

For shift/reduce conflicts, the conflict is expected if all instances of the
shift token are marked with a conflict marker, and the rule to be reduced is
marked with a conflict marker.  Note that during construction of the discrete
finite automaton multiple locations in the source grammar are collapsed into
a single state, so multiple instances of a token may need to be marked.

For reduce/reduce conflicts, the conflict is expected if both rules are marked
with the conflict marker.


### Generalized parsing

A grammar with unresolved conflicts is effectively ambiguous.  Such grammars
either require more than one token of lookahead, or additional contextual
knowledge must be used to disambiguate two or more valid parse trees.  GLR
parsing allows pomelo to handle both these cases - unambiguous grammars that
are not LALR(1), as well as truly ambiguous grammars.

When the parser encounters an expected conflict, the parser is split into
two (or more) independent parsers.  Instead of a single parser stack, there
are now multiple parser stacks, each with their own stack top, user value, and
state.

The user value for the split stack is generated by a call to the `%user_split`
function.

    %user_split
    {
        return split_parse( u );
    }


This function is called with one argument, `u`, which is a reference to the
user value from the original stack.  `u` will be used as the user value for the
first stack.  The function must return a user value for the second stack.

The part of the parser stack that is common is shared.

                                  <- type_name LT NAME (u, state 52)
        func_header LBR stmt_list
                                  <- decl_name LT NAME (v, state 98)

In pomelo, the parse stack is a tree structure, not a graph - only the left
context is shared.  This is sufficient for languages where ambiguities are
fairly localised, and alternatives either die off or merge quickly.

If the parser encounters an error, and there is more than one valid parse
remaining, the parse that errored is destroyed, and parsing continues.

Ambiguous grammars can merge alternatives using a merge function.  This is a
block of code attached to a nonterminal production with the `@` symbol.  If
two parses reduce to this nonterminal at the same time with identical left
contexts and identical states, then the parses are merged.

    ambiguous_expr_decl { node_ptr }
        @{ return merge_parses( u, a, v, b ); }
    [
        expr(e) . { return e; }
        decl(d) . { return d; }
    ]

The parameters passed to a merge function are:

  * `u` : A reference to the user value from the first parse.  This becomes the
    user value of the merged parse stack.

  * `a` : An rvalue reference to the nonterminal value from the first parse.
    The function consumes this value during the merge.

  * `v` : An rvalue reference to the user value from the second parse.   The
    function consumes this value during the merge, and the second parse stack
    is destroyed by the merge operation.

  * `b` : An rvalue reference to the nonterminal value from the second parse.
    The function consumes this value during the merge.

It should return a single value which is used as the value of the nonterminal
in the merged parse stack.


### Error Reporting

The error function is defined using the `%error_report` directive.

    %error_report
    {
        fprintf( stderr, "unexpected token '%s'\n", symbol_name( token ) );
    }
    
When there is only one valid parse, and the parser is given a unexpected token,
then the error function is called with the following arguments:

  * `u` : A reference to the user value for the current parse.

  * `token` : The token number (from the enumeration) of the unexpected token.

  * `tokval` : A reference to the token's value.

Currently the parser does not attempt error recovery - the parser remains in
the same state after reporting an unexpected token.


### Directives

Directives supported in syntax files:

  * `%include { /* C++ */ }` or `%include_header { /* C++ */ }` : Places a
    block of C++ code at the top of the generated header file.  Allows
    inclusion of headers or declarations of types for user values.

  * `%include_source { /* C++ */ }` : Places a block of C++ code at the top of
    the generated source file.  Useful for including definitions of token or
    user value types, or other declarations needed by parser actions.

  * `%user_value { type_name }` : Specify the type of the user value
    associated with each parse.

  * `%user_split { /* C++ */ }` : Declares the split function for user values,
    see above.

  * `%class_name { identifier }` : Give a name for the generated parser class.
    The default is `parser`.
    
  * `%token_type { type_name }` : The value type for terminal symbols.  Like
    value types for nonterminals, terminal values are usually moved, but they
    are copied into the parser for each live parse, and may be copied when
    stacks are split.

  * `%token_prefix { PREFIX_ }` : An enumeration listing all terminal symbols
    (and giving them an token number) is written into the generated header.
    This allows you to provide a prefix for the enumerators.

  * `%nterm_prefix { PREFIX_ }` : An enumeration listing all nonterminal
    symbols is written into the generated header.  This allows you to provide
    a prefix for the enumerators.  Note that nonterminal names are converted
    to upper case in this context.

  * `%error_report { /* C++ */ }` : Declares the error function, see above.

  * `%left`, `%right`, `%nonassoc` : Assigns precedence to terminal symbols,
    see above.


## TODO

* Implement an error recovery strategy for the generated parser.


## License

Copyright © 2018 Edmund Kapusniak.  Licensed under the MIT License. See
LICENSE file in the project root for full license information. 

