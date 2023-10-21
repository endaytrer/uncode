#!/usr/bin/env python
import re
import os
import sys
from typing import Literal, Callable, NamedTuple

SHADERS = "shaders"
OUTPUT = sys.argv[1]

pattern = re.compile(r'^(layout\s*\(\s*location\s*=\s*(\d+)\)\s+)?in\s+(float|vec2|vec3|vec4)\s+([A-Za-z_][A-Za-z0-9_]*?)\s*;$')

VAR_TYPE_TYPE = Literal['float', 'vec2', 'vec3', 'vec4']
TYPE_CONVERSION: dict[VAR_TYPE_TYPE, Callable[[str], str]] = {
    "float": lambda s: f"float {s};",
    "vec2": lambda s: f"float {s}[2];",
    "vec3": lambda s: f"float {s}[3];",
    "vec4": lambda s: f"float {s}[4];"
}
Variable = NamedTuple("Variable", [('location', int), ('type', VAR_TYPE_TYPE), ('name', str)])

header = """
#ifndef _RENDERABLES_H
#define _RENDERABLES_H

/* GENERATED BY generate_renderables.py */
#include <stddef.h>

"""
trailer = """
#endif // _RENDERABLES_H
"""
if __name__ == "__main__":
    


    with open(OUTPUT, 'w') as of:
        of.write(header)
        filenames: list[str] = []
        for filename in os.listdir(SHADERS):
            if not filename.endswith(".glsl"):
                continue

            filenames.append(SHADERS + '_' + filename.replace('.', '_'))

            if not filename.endswith(".vert.glsl"):
                continue


            typename = filename.split(".")[0].strip()
            current_location = 0
            variables: list[Variable] = []
            with open(SHADERS + '/' + filename, "r") as f:
                for line in f:
                    s = line.split("//")[0]
                    s = s.strip()
                    if line == '':
                        continue
                    matches = pattern.fullmatch(s)
                    if matches is None:
                        continue
                    
                    if matches[2] is not None:
                        current_location = int(matches[2])
                    type = matches[3]
                    name = matches[4]
                    variables.append(Variable(current_location, type, name))
                    current_location += 1
            # structure itself

            of.write("typedef struct {\n")
            for v in variables:
                of.write(f"    {TYPE_CONVERSION[v.type](v.name)}\n")
            of.write("} " + typename.capitalize() + ";\n\n")

#             # attrib location
#             of.write("typedef enum {\n")
#             for v in variables:
#                 of.write(f"    {typename.upper()}_{v.name.upper()} = {v.location},\n")
#             of.write(f"    {typename.upper()}_ATTR_COUNT\n")
#             of.write("} " + typename.capitalize() + "Attr;\n\n")
            
#             # attrib param
#             of.write(f"static const AttrParam {typename.lower()}_attr_param[{typename.upper()}_ATTR_COUNT] = {{\n")
#             for v in variables:
#                 of.write(f"""    [{typename.upper()}_{v.name.upper()}] = {{
#         .size = sizeof((({typename.capitalize()} *)NULL)->{v.name}) / sizeof(float),
#         .offset = offsetof({typename.capitalize()}, {v.name})
#     }},
# """)
#             of.write("};\n\n")

            # auto filler macro
            of.write(f"#define FILL_ATTR_{typename.upper()}() \\\ndo {{\\\n")
            for v in variables:
                of.write(f"""    glVertexAttribPointer( \\
        {v.location}, \\
        sizeof((({typename.capitalize()} *)NULL)->{v.name}) / sizeof(float), \\
        GL_FLOAT, \\
        GL_FALSE, \\
        sizeof({typename.capitalize()}), \\
        (void *)offsetof({typename.capitalize()}, {v.name})\\
    ); \\
    glEnableVertexAttribArray({v.location}); \\
    glVertexAttribDivisor({v.location}, 1); \\
""")
            of.write("} while (0)\n\n")
        


        for name in filenames:
            of.write(f"extern unsigned char {name}[];\nextern unsigned int {name}_len;\n\n")
        of.write(trailer)