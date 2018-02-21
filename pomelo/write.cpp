//
//  write.cpp
//  pomelo
//
//  Created by Edmund Kapusniak on 02/02/2018.
//  Copyright © 2018 Edmund Kapusniak. All rights reserved.
//


#include "write.h"
#include <assert.h>
#include <string_view>


namespace
{
#include "template.inc"
}



write::write( automata_ptr automata, action_table_ptr action_table,
                goto_table_ptr goto_table, const std::string& output_h )
    :   _automata( automata )
    ,   _action_table( action_table )
    ,   _goto_table( goto_table )
    ,   _output_h( output_h )
{
}

write::~write()
{
}


std::string write::trim( const std::string& s )
{
    size_t lower = s.find_first_not_of( " \t" );
    size_t upper = s.find_last_not_of( " \t" );
    if ( lower != std::string::npos && upper != std::string::npos )
    {
        return s.substr( lower, upper + 1 - lower );
    }
    else
    {
        return "";
    }
}

bool write::starts_with( const std::string& s, const std::string& z )
{
    return s.compare( 0, z.size(), z ) == 0;
}


void write::prepare()
{
    // Munge header name.
    size_t pos = _output_h.find_last_of( "/\\" );
    if ( pos != std::string::npos )
    {
        _output_h = _output_h.substr( pos + 1 );
    }

    // Build in-order list of terminals.
    for ( const auto& tsym : _automata->syntax->terminals )
    {
        _tokens.push_back( tsym.second.get() );
    }
    std::sort
    (
        _tokens.begin(),
        _tokens.end(),
        []( terminal* a, terminal* b ) { return a->value < b->value; }
    );

    // Build in-order list of nonterminals.
    for ( const auto& nsym : _automata->syntax->nonterminals )
    {
        _nterms.push_back( nsym.second.get() );
    }
    std::sort
    (
        _nterms.begin(),
        _nterms.end(),
        []( nonterminal* a, nonterminal* b ) { return a->value < b->value; }
    );
    
    
    // If there is no user-specified token type, then add the empty type.
    std::unordered_map< std::string, ntype* > lookup;
    if ( trim( _automata->syntax->token_type.text ).empty() )
    {
        std::unique_ptr< ntype > n = std::make_unique< ntype >();
        n->ntype = "empty";
        n->value = (int)_ntypes.size();
        ntype* resolved = n.get();
        lookup.emplace( "empty", resolved );
        _ntypes.push_back( std::move( n ) );
    }
    
    // Work out nonterminal types for each nonterminal.
    for ( nonterminal* nterm : _nterms )
    {
        std::string type = trim( nterm->type );
        if ( nterm->type.empty() )
        {
            type = "empty";
        }
    
        ntype* resolved;
        auto i = lookup.find( type );
        if ( i != lookup.end() )
        {
            resolved = i->second;
        }
        else
        {
            std::unique_ptr< ntype > n = std::make_unique< ntype >();
            n->ntype = type;
            n->value = (int)_ntypes.size();
            resolved = n.get();
            lookup.emplace( type, resolved );
            _ntypes.push_back( std::move( n ) );
        }
        
        _nterm_lookup.emplace( nterm, resolved );
    }
}





/*
    Here are the interpolated values we fill in in the template:
 
        $(include)
        $(class_name)
        $(user_value)
        $(token_type)
        $(start_state)
        $(error_action)
        $(accept_action)
        $(token_count)
        $(nterm_count)
        $(state_count)
        $(rule_count)
        $(conflict_count)
        $(error_report)
 
    Tables:
 
        $(action_table)
        $(conflict_table)
        $(goto_table)
        $(rule_table)
 
    Per-token:
 
        $$(token_name)
        $$(token_value)
 
    Per-non-terminal:
 
        $$(nterm_name)
        $$(nterm_value)
 
    Per-non-terminal-type:
 
        $$(ntype_type)
        $$(ntype_value)
 
    Per-rule:
 
        $$(rule_type)
        $$(rule_name)
        $$(rule_param)
        $$(rule_body)
        $$(rule_assign)
        $$(rule_args)
 
*/


bool write::write_header( FILE* f )
{
    return write_template( f, (char*)template_h, template_h_len, true );
}

bool write::write_source( FILE* f )
{
    return write_template( f, (char*)template_cpp, template_cpp_len, false );
}


bool write::write_template( FILE* f, char* source, size_t length, bool header )
{
    // Process file line-by-line.
    size_t i = 0;
    while ( i < length )
    {
        // Get to end of line.
        size_t iend = i;
        while ( iend < length )
        {
            if ( source[ iend ] == '\n' )
            {
                iend += 1;
                break;
            }
            iend += 1;
        }
        
        // Get line.
        std::string line( source + i, iend - i );
        std::string output;
        
        if ( line[ 0 ] == '?' || line[ 0 ] == '!' )
        {
            syntax_ptr syntax = _automata->syntax;

            bool skip = false;

            if ( starts_with( line, "?(user_value)" ) )
            {
                skip = skip || trim( syntax->user_value.text ).empty();
                line = line.substr( line.find( ')' ) + 1 );
            }
            if ( starts_with( line, "!(user_value)" ) )
            {
                skip = skip || ! trim( syntax->user_value.text ).empty();
                line = line.substr( line.find( ')' ) + 1 );
            }
            if ( starts_with( line, "?(token_type)" ) )
            {
                skip = skip || trim( syntax->token_type.text ).empty();
                line = line.substr( line.find( ')' ) + 1 );
            }
            if ( starts_with( line, "!(token_type)" ) )
            {
                skip = skip || ! trim( syntax->token_type.text ).empty();
                line = line.substr( line.find( ')' ) + 1 );
            }

            if ( skip )
            {
                i = iend;
                continue;
            }
        }

        // Check for interpolants.
        size_t per = line.find( "$$(" );
        if ( per != std::string::npos )
        {
            if ( line.compare( per, 9, "$$(token_" ) == 0 )
            {
                for ( terminal* token : _tokens )
                {
                    if ( token->is_special )
                    {
                        continue;
                    }
                    output += replace( replace( line, token ) );
                }
            }
            else if ( line.compare( per, 9, "$$(nterm_" ) == 0 )
            {
                for ( nonterminal* nterm : _nterms )
                {
                    if ( nterm->is_special )
                    {
                        continue;
                    }
                    output += replace( replace( line, nterm ) );
                }
            }
            else if ( line.compare( per, 9, "$$(ntype_" ) == 0 )
            {
                for ( const auto& ntype : _ntypes )
                {
                    output += replace( replace( line, ntype.get() ) );
                }
            }
            else if ( line.compare( per, 8, "$$(rule_" ) == 0 )
            {
                for ( const auto& rule : _automata->syntax->rules )
                {
                    output += replace( replace( line, rule.get(), header ) );
                }
            }
            else
            {
                assert( ! "invalid template" );
            }
        }
        else if ( line.find( "$(" ) != std::string::npos )
        {
            output = replace( line );
        }
        else
        {
            output = line;
        }
        
        if ( fwrite( output.data(), 1, output.size(), f ) < output.size() )
        {
            fprintf( stderr, "error writing output file" );
            return false;
        }
        
        i = iend;
    }
    
    return true;
}


struct replacer
{
    std::string& line;
    const char* prefix;
    size_t i;
    size_t lower;
    size_t length;
    
    replacer( std::string& line, const char* prefix )
        :   line( line )
        ,   prefix( prefix )
        ,   i( 0 )
        ,   lower( 0 )
        ,   length( 0 )
    {
    }
    
    bool next( std::string_view& valname )
    {
        i = lower;
        if ( ( lower = line.find( prefix, i ) ) != std::string::npos )
        {
            size_t upper = line.find( ')', lower );
            assert( upper != std::string::npos );
            length = upper + 1 - lower;
            valname = std::string_view( line ).substr( lower, length );
            return true;
        }
        else
        {
            return false;
        }
    }
    
    void replace( const std::string& s )
    {
        line.replace( lower, length, s );
    }
};


std::string write::replace( std::string line )
{
    /*
        $(include)
        $(header)
        $(class_name)
        $(user_value)
        $(user_split)
        $(token_type)
        $(start_state)
        $(error_action)
        $(accept_action)
        $(token_count)
        $(nterm_count)
        $(state_count)
        $(rule_count)
        $(conflict_count)
        $(error_report)

        $(action_table)
        $(conflict_table)
        $(goto_table)
        $(rule_table)
    */
    
    syntax_ptr syntax = _automata->syntax;
    replacer r( line, "$(" );
    std::string_view valname;
    while ( r.next( valname ) )
    {
        if ( valname == "$(include)" )
        {
            r.replace( trim( syntax->include.text ) );
        }
        else if ( valname == "$(header)" )
        {
            r.replace( _output_h );
        }
        else if ( valname == "$(class_name)" )
        {
            std::string name = trim( syntax->class_name.text );
            if ( name.empty() )
            {
                name = "parser";
            }
            r.replace( name );
        }
        else if ( valname == "$(user_value)" )
        {
            r.replace( trim( syntax->user_value.text ) );
        }
        else if ( valname == "$(user_split)" )
        {
            std::string split = trim( syntax->user_split.text );
            if ( split.empty() )
            {
                split = "return u;";
            }
            r.replace( split );
        }
        else if ( valname == "$(token_type)" )
        {
            r.replace( trim( syntax->token_type.text ) );
        }
        else if ( valname == "$(start_state)" )
        {
            r.replace( std::to_string( _automata->start->index ) );
        }
        else if ( valname == "$(error_action)" )
        {
            r.replace( std::to_string( _action_table->error_action ) );
        }
        else if ( valname == "$(accept_action)" )
        {
            r.replace( std::to_string( _action_table->accept_action ) );
        }
        else if ( valname == "$(token_count)" )
        {
            r.replace( std::to_string( _action_table->token_count ) );
        }
        else if ( valname == "$(nterm_count)" )
        {
            r.replace( std::to_string( _goto_table->nterm_count ) );
        }
        else if ( valname == "$(state_count)" )
        {
            r.replace( std::to_string( _action_table->state_count ) );
        }
        else if ( valname == "$(rule_count)" )
        {
            r.replace( std::to_string( _action_table->rule_count ) );
        }
        else if ( valname == "$(conflict_count)" )
        {
            r.replace( std::to_string( _action_table->conflict_count ) );
        }
        else if ( valname == "$(error_report)" )
        {
            r.replace( trim( syntax->error_report.text ) );
        }
        else if ( valname == "$(action_table)" )
        {
            r.replace( write_table( _action_table->actions ) );
        }
        else if ( valname == "$(conflict_table)" )
        {
            r.replace( write_table( _action_table->conflicts ) );
        }
        else if ( valname == "$(goto_table)" )
        {
            r.replace( write_table( _goto_table->gotos ) );
        }
        else if ( valname == "$(rule_table)" )
        {
            r.replace( write_rule_table() );
        }
        else
        {
            fprintf( stdout, "%.*s", (int)valname.size(), valname.data() );
            fflush( stdout );
            assert( ! "invalid template" );
        }
    }
    
    return line;
}

std::string write::replace( std::string line, terminal* token )
{
    /*
        $$(token_name)
        $$(raw_token_name)
        $$(token_value)
    */

    syntax_ptr syntax = _automata->syntax;
    replacer r( line, "$$(" );
    std::string_view valname;
    while ( r.next( valname ) )
    {
        if ( valname == "$$(token_name)" )
        {
            std::string name = syntax->source->text( token->name );
            std::string prefix = trim( syntax->token_prefix.text );
            if ( prefix.empty() )
            {
                prefix = "TOKEN_";
            }
            r.replace( prefix + name );
        }
        else if ( valname == "$$(raw_token_name)" )
        {
            std::string name = syntax->source->text( token->name );
            r.replace( name );
        }
        else if ( valname == "$$(token_value)" )
        {
            r.replace( std::to_string( token->value ) );
        }
        else
        {
            assert( ! "invalid template" );
        }
    }

    return line;
}

std::string write::replace( std::string line, nonterminal* nterm )
{
    /*
        $$(nterm_name)
        $$(raw_nterm_name)
        $$(nterm_value)
    */
    
    syntax_ptr syntax = _automata->syntax;
    replacer r( line, "$$(" );
    std::string_view valname;
    while ( r.next( valname ) )
    {
        if ( valname == "$$(nterm_name)" )
        {
            std::string name = syntax->source->text( nterm->name );
            std::transform( name.begin(), name.end(), name.begin(), ::toupper );
            std::string prefix = trim( syntax->nterm_prefix.text );
            if ( prefix.empty() )
            {
                prefix = "NTERM_";
            }
            r.replace( prefix + name );
        }
        else if ( valname == "$$(raw_nterm_name)" )
        {
            std::string name = syntax->source->text( nterm->name );
            r.replace( name );
        }
        else if ( valname == "$$(nterm_value)" )
        {
            r.replace( std::to_string( nterm->value ) );
        }
        else
        {
            assert( ! "invalid template" );
        }
    }

    return line;
}

std::string write::replace( std::string line, ntype* ntype )
{
    /*
        $$(ntype_type)
        $$(ntype_value)
    */
    
    replacer r( line, "$$(" );
    std::string_view valname;
    while ( r.next( valname ) )
    {
        if ( valname == "$$(ntype_type)" )
        {
            r.replace( ntype->ntype );
        }
        else if ( valname == "$$(ntype_value)" )
        {
            r.replace( std::to_string( ntype->value ) );
        }
        else
        {
            assert( ! "invalid template" );
        }
    }

    return line;
}

std::string write::replace( std::string line, rule* rule, bool header )
{
    /*
        $$(rule_type)
        $$(rule_name)
        $$(rule_param)
        $$(rule_body)
        $$(rule_index)
        $$(rule_assign)
        $$(rule_args)
    */

    syntax_ptr syntax = _automata->syntax;
    replacer r( line, "$$(" );
    std::string_view valname;
    while ( r.next( valname ) )
    {
        if ( valname == "$$(rule_type)" )
        {
            std::string ntype = _nterm_lookup.at( rule->nterm )->ntype;
            if ( ! header && ntype == "empty" )
            {
                std::string name = trim( syntax->class_name.text );
                if ( name.empty() )
                {
                    name = "parser";
                }
                ntype = name + "::" + ntype;
            }
            r.replace( ntype );
        }
        else if ( valname == "$$(rule_name)" )
        {
            r.replace( std::string( "rule_" ) + std::to_string( rule->index ) );
        }
        else if ( valname == "$$(rule_param)" )
        {
            std::string prm;
            if ( trim( syntax->user_value.text ).size() )
            {
                prm += " const user_value& u";
            }
            for ( size_t i = 0; i < rule->locount - 1; ++i )
            {
                size_t iloc = rule->lostart + i;
                const location& loc = syntax->locations.at( iloc );
                if ( ! loc.sparam )
                {
                    continue;
                }

                if ( prm.size() )
                {
                    prm += ",";
                }
                if ( loc.sym->is_terminal )
                {
                    prm += " token_type&&";
                }
                else
                {
                    prm += " ";
                    nonterminal* nterm = (nonterminal*)loc.sym;
                    prm += _nterm_lookup.at( nterm )->ntype;
                    prm += "&&";
                }
                prm += " ";
                prm += syntax->source->text( loc.sparam );
            }
            if ( prm.size() )
            {
                prm += " ";
            }

            r.replace( prm );
        }
        else if ( valname == "$$(rule_body)" )
        {
            std::string body = trim( rule->action );
            if ( body.empty() )
            {
                body = "return {};";
            }
            r.replace( body );
        }
        else if ( valname == "$$(rule_index)" )
        {
            r.replace( std::to_string( rule->index ) );
        }
        else if ( valname == "$$(rule_assign)" )
        {
            if ( rule->locount > 1 )
            {
                r.replace( "p[ 0 ] = value( p[ 0 ].state(), " );
            }
            else
            {
                r.replace( "values.emplace_back( s->state, " );
            }
        }
        else if ( valname == "$$(rule_args)" )
        {
            std::string args;
            if ( trim( syntax->user_value.text ).size() )
            {
                args += " s->u";
            }
            for ( size_t i = 0; i < rule->locount - 1; ++i )
            {
                size_t iloc = rule->lostart + i;
                const location& loc = syntax->locations.at( iloc );
                if ( ! loc.sparam )
                {
                    continue;
                }

                if ( args.size() )
                {
                    args += ",";
                }
                args += " p[ ";
                args += std::to_string( i );
                args += " ].move()";
            }
            if ( args.size() )
            {
                args += " ";
            }
        
            r.replace( args );
        }
        else
        {
            assert( ! "invalid template" );
        }
    }

    return line;
}


std::string write::write_table( const std::vector< int >& table )
{
    std::string s;
    for ( size_t i = 0; i < table.size(); ++i )
    {
        if ( i % 20 == 0 )
        {
            if ( i > 0 )
            {
                s += "\n";
            }
            s += "   ";
        }
        s += " ";
        s += std::to_string( table.at( i ) );
        s += ",";
    }
    s += "\n";
    return s;
}


std::string write::write_rule_table()
{
    int token_count = (int)_automata->syntax->terminals.size();
    source_ptr source = _automata->syntax->source;

    std::string s;
    for ( size_t i = 0; i < _automata->syntax->rules.size(); ++i )
    {
        rule* rule = _automata->syntax->rules.at( i ).get();
        s += "    { ";
        s += std::to_string( rule->nterm->value - token_count );
        s += ", ";
        s += std::to_string( (int)rule->locount - 1 );
        s += " }, // ";

        s += source->text( rule->nterm->name );
        s += " :";
        for ( size_t i = 0; i < rule->locount - 1; ++i )
        {
            size_t iloc = rule->lostart + i;
            const location& loc = _automata->syntax->locations.at( iloc );
            s += " ";
            s += source->text( loc.sym->name );
        }
        
        s += "\n";
    }
    
    return s;
}


