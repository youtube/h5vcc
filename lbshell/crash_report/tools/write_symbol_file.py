#!/usr/bin/python

# Copyright 2013 Google Inc. All Rights Reserved.
# This is a small script to transfer a PS3 SNS map file to breakpad symbol file.

# Reference: https://code.google.com/p/google-breakpad/wiki/SymbolFiles

import hashlib
import re
import sys

class Range:
  def __init__(self, address, size):
    self.address = address
    self.size = size

  def contains(self, range):
    if (range.size == 0):
      if (self.address <= range.address) and \
         (self.address + self.size > range.address):
        return True
      else:
        return False
    if (self.address <= range.address) and \
       (self.address + self.size >= range.address + range.size):
      return True
    return False

  def overlaps(self, range):
    if (self.address >= range.address + range.size) or \
       (self.address + self.size <= range.address):
      return False
    return True

def md5_check_sum(filePath):
  fh = open(filePath, 'rb')
  m = hashlib.md5()
  while True:
    data = fh.read(8192)
    if not data:
      break
    m.update(data)
  fh.close()
  return m.hexdigest().upper()


def write_symbol_file(input_file, output_file):
  symbol_regex = re.compile(
      '([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+(.*)')
  identifier = md5_check_sum(input_file)

  f_input = open(input_file, 'r')
  f_output = open(output_file, 'w')

  # Since breakpad requires to choose operating system from Linux/mac/windows,
  # we used Linux here although it is not true.
  # Added one '0' to the end of identifier since breakpad symbol identifier
  # is GUID + age(revision of the debug file).
  f_output.write('MODULE Linux ppc64 ' + identifier + '0' +
      ' steel.lb_shell.ppu\n')

  # Find the .text and remember the memory region of .text section.
  text_start_addr = 0
  text_end_addr = 0
  lines = f_input.readlines()
  for line in lines:
    symbol_match = symbol_regex.match(line)
    if not symbol_match:
      continue
    data = {'address': symbol_match.groups()[0],
            'size': symbol_match.groups()[1],
            'name': symbol_match.groups()[-1]}
    if data['name'].strip() == '.text':
      text_range = Range(int(data['address'], 16), int(data['size'], 16))
      break

  func = []
  files = []
  lines_info = []
  last_appended_is_file = False
  last_data = None

  for line in lines:
    symbol_match = symbol_regex.match(line)
    if not symbol_match:
      continue
    data = {'address': symbol_match.groups()[0],
            'size': symbol_match.groups()[1],
            'name': symbol_match.groups()[-1]}
    data_range = Range(int(data['address'], 16), int(data['size'], 16))
    if not text_range.contains(data_range):
      continue

    # To determine if the data is function or file, using the following rule:
    # If the last line (i-1) of data address range covers the range of this
    # line (i) of data, the last line is determined to be file record. If the
    # last line address range does not overlap with the range of this line,
    # last line is determined to be function record if it is covered by the
    # address range of last file, otherwise last line is determined to be file
    # record. If the two consecutive lines of file determined, and the former
    # one covers the latter, the former one is determined to be one level above
    # and deleted from the file record list.
    if last_data is not None and last_range is not None:
      if last_range.contains(data_range):
        if last_appended_is_file and \
           last_file_range.contains(last_range):
          files.pop()
        files.append(last_data)
        last_appended_is_file = True
        last_file_range = last_range
        last_file_name = last_data['name']
      elif not last_range.overlaps(data_range):
        if last_file_range.contains(last_range):
          last_data['parameter_size'] = str(0)
          last_data['name'] = last_file_name.rstrip() + '::' + last_data['name']
          func.append(last_data)
          lines_info.append({'address': last_data['address'],
                             'size': last_data['size'],
                             'line': str(1),
                             'filenum': str(len(files))})
          last_appended_is_file = False
        else:
          files.append(last_data)
          last_appended_is_file = True
          last_file_range = last_range
          last_file_name = last_data['name']
      else:
        last_appended_is_file = False
    last_data = data
    last_range = data_range

  # add last line data
  last_data['parameter_size'] = str(0)
  func.append(last_data)
  lines_info.append({'address': last_data['address'],
                     'size': last_data['size'],
                     'line': str(1),
                     'filenum': str(len(files))})

  for i in range(len(files)):
    f_output.write('FILE %s %s\n' % (str(i), files[i]['name']))

  for f in func:
    f_output.write('FUNC %s %s %s %s\n' % (f['address'], f['size'],
          f['parameter_size'], f['name']))

  for l in lines_info:
    f_output.write('%s %s %s %s\n' % (l['address'], l['size'],
                                      l['line'], l['filenum']))

  f_input.close()
  f_output.close()


if __name__ == "__main__":
  if len(sys.argv) < 3:
    print 'Usage: {0} <lb_shell.ppu.map> <output_file>'.format(sys.argv[0])
    sys.exit(0)
  try:
    write_symbol_file(sys.argv[1], sys.argv[2])
  except(IOError, SyntaxError), msg:
    print "Error:", msg
    sys.exit(1)
