#
#  pomelo
#
#  Created by Edmund Kapusniak on 18/02/2018.
#  Copyright © 2018 Edmund Kapusniak. All rights reserved.
#


project( 'pomelo', 'cpp', default_options: [ 'cpp_std=gnu++17' ] )

sources = [
    'pomelo/actions.cpp',
    'pomelo/automata.cpp',
    'pomelo/errors.cpp',
    'pomelo/lalr1.cpp',
    'pomelo/main.cpp',
    'pomelo/options.cpp',
    'pomelo/parser.cpp',
    'pomelo/search.cpp',
    'pomelo/syntax.cpp',
    'pomelo/token.cpp',
    'pomelo/write.cpp'
]

sources += custom_target(
    'templateinc',
    input : [ 'pomelo/template.h', 'pomelo/template.cpp' ],
    output : [ 'template.inc' ],
    command : [ find_program( 'geninc.sh' ), '@INPUT0@', '@INPUT1@', '@OUTPUT@' ]
)

executable( 'pomelo', sources : sources, install : true )

