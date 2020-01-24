#!/usr/bin/env python3

import sys
import os

arguments = sys.argv[ 1 : ]
output = arguments.pop()

with open( output, 'w' ) as file:

    file.write( "#include <stddef.h>\n" )

    for input in arguments:

        varname = os.path.basename( input ).replace( ".", "_" )

        with open( input, 'rb' ) as infile:
            data = infile.read()

        file.write( f"unsigned char { varname }[] = {{" )
        count = 0
        for byte in data:
            if count % 16 == 0:
                file.write( "\n " )
            file.write( f" 0x{ byte :02X}," )
            count += 1
        file.write( "\n};\n" )
        file.write( f"size_t { varname }_len = { len( data ) };\n" )

