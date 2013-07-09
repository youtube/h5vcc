#!/usr/bin/env python

'''
Usage:  extract_shaders.py [OUTPUT_DIR VERTEX_EXT FRAGMENT_EXT]

Called by itself, the script will open the file at shader_source_path and
extract all the shader code from it, printing the results to standard output.
If OUTPUT_DIR argument is provided, the shaders will get written to shader files
in OUTPUT_DIR.  In this case, VERTEX_EXT and FRAGMENT_EXT must also be
specified, and they represent the extension that should be given to vertex
shaders and fragment shaders, respectively.
'''

import os
import sys
# for uint32_t hash calculation
import ctypes

# hard-coded path to the Chromium file that contains all of Chrome's GLSL
# shaders
shader_source_path = '../../external/chromium/cc/shader.cc'
# note that the SHADER macro is currently defined at the top of the file, but
# all instances of that macro have a newline suffix. If subsequent revisions
# break that assumption we will have to revist
shader_search_key = 'SHADER(\n'

# returns a list of all strings within SHADER() macros in the given file
def ExtractShaderSources(file_path):
  f = open(file_path)
  cpp_source = f.read()
  f.close()

  # Keep a mapping of source
  source_list = []
  shader_start = cpp_source.find(shader_search_key)
  while shader_start > 0:
    # scan forward to find closing paren
    paren_nest_depth = 1
    shader_stop = shader_start + len(shader_search_key)
    while paren_nest_depth > 0:
      if cpp_source[shader_stop] == '(':
        paren_nest_depth += 1
      elif cpp_source[shader_stop] == ')':
        paren_nest_depth -= 1
      shader_stop += 1

    # try to find the name of the shader class where this shader was defined
    class_name = '?'
    get_shader_string = cpp_source.rfind('::getShaderString', 0, shader_start)
    if get_shader_string != -1:
      class_name_start = cpp_source.rfind(' ', 0, get_shader_string)
      if class_name_start != -1:
        class_name = cpp_source[class_name_start:get_shader_string].strip()

    # we found the closing paren, add to shader source list
    source_list.append((class_name, cpp_source[shader_start : shader_stop]))
    # find next shader in file
    shader_start = cpp_source.find(shader_search_key, shader_stop)
  # strip off leading and trailing macro, paren, and whitespace
  source_list = [(s[0], s[1][len(shader_search_key):-1].strip("\n\t "))
    for s in source_list]
  # strip out internal whitespace and newlines, replace with a single newline
  source_list = [(s[0], '\n'.join([x.strip() for x in s[1].splitlines()]))
    for s in source_list]

  return source_list

def add_uint32(a, b):
  return (a + b) & 0xffffffff

def xor_uint32(a, b):
  return (a ^ b) & 0xffffffff

# returns a numeric hash of the  provided source string calculated in the same
# manner as the shader emulation code in lb_web_graphics_context_3d.cc
def HashShaderSource(shader):
  hash = 0
  for c in shader.replace('\n', ' '):
    hash = add_uint32(hash, ord(c))
    hash = add_uint32(hash, hash << 10)
    hash = xor_uint32(hash, hash >> 6)
  hash = add_uint32(hash, hash << 3)
  hash = xor_uint32(hash, hash >> 11)
  hash = add_uint32(hash, hash << 15)
  return hash

def main(args):
  shaders = ExtractShaderSources(shader_source_path)

  # If an argument is passed, shader files are automatically dumped in
  # to the directory specified by the argument
  if len(args) > 0:
    out_dir = args[0]
    vs_ext = args[1]
    fs_ext = args[2]

    for s in shaders:
      prefix = 'unknown'
      ext = 'unknown'
      if s[0].find('Fragment') == 0:
        prefix = 'fragment'
        ext = fs_ext
      elif s[0].find('Vertex') == 0:
        prefix = 'vertex'
        ext = vs_ext

      shader_file = prefix + '_' + ('%08x'%HashShaderSource(s[1])) + '.' + ext
      f = open(os.path.join(out_dir, shader_file), 'w')
      f.write('// ' + s[0] + '\n\n')
      f.write(s[1])
      f.close()
  else:
    for s in shaders:
      print '%s (%08x): %s\n' % (s[0], HashShaderSource(s[1]), s[1])



if __name__ == '__main__':
  args = sys.argv[1:]
  if len(args) != 0 and len(args) != 3:
    print __doc__
    sys.exit(-1)

  main(args)
