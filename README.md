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
    rontend.
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


## Syntax files

### Directives




### Productions

Syntax files consist of a list of nonterminal productions.  Each production
consists of its name, followed by a list of rules enclosed in square brackets.

    nonterminal_name
    [
        A B C .
    ]

Nonterminal (production) names are lower-case.  Terminal (token) names are
upper-case.  All rules for a particular production must be specified at the 
ame time.

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

Action code must return a value for the resulting nonterminal value.  The type
of a nonterminal is specified in curly braces after the nonterminal name.

    pp_qq_opt { node_ptr }
    [
        .
            { return node_ptr(); }
        PP contents(c) DOT(op) qual_name(name) QQ .
            { return build_node( c, op, name); }
    ]


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
have higher precedence.  Tokens which appear in the same directive have the
same precedence.

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

Many real grammars are not LALR(1) and will contain unresolved conflicts.  This
is why pomelo implements generalized parsing.  By default, pomelo reports an
error (and returns a failure code) when it encounters unresolved conflicts in a
grammar.

When writing a grammar, especially for an LR parser, conflicts are not always
obvious.  It is easy to edit a grammar and introduce ambiguity where none was
intended.  However, some ambiguities are real features of the language and
should be accepted by the parser generator - they will trigger a split in the
parser stack at runtime.

Therefore, a grammar author must mark expected unresolved conflicts using
the conflict marker `!` where they appear in the grammar.

To mark a shift as conflicting, place a conflict marker before the token that
will be shifted where it appears in the rule.

To mark a rule as conflicting, place a conflict marker at the end of the rule,
before the period.

    conflicts_example
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


## Generated parsers


## Generalized parsing


## TODO

* Implement an error recovery strategy for the generated parser.
* Compress the parser tables.


## License

Copyright © 2018 Edmund Kapusniak.  Licensed under the MIT License. See
LICENSE file in the project root for full license information. 

