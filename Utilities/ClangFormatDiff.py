#!/usr/bin/env python
#
#===- clang-format-diff.py - ClangFormat Diff Reformatter ----*- python -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

r"""
ClangFormat Diff Reformatter
============================

This script reads input from a unified diff and reformats all the changed
lines. This is useful to reformat all the lines touched by a specific patch.
Example usage for git/svn users:

  git diff -U0 --no-color HEAD^ | clang-format-diff.py -p1 -i
  svn diff --diff-cmd=diff -x-U0 | clang-format-diff.py -i

"""
from __future__ import absolute_import, division, print_function

import argparse
import difflib
import re
import subprocess
import sys

if sys.version_info.major >= 3:
  from io import StringIO
else:
  from io import BytesIO as StringIO


def main():
  parser = argparse.ArgumentParser(description=
                                   'Reformat changed lines in diff. Without -i '
                                   'option just output the diff that would be '
                                   'introduced.')
  parser.add_argument('-i', action='store_true', default=False,
                      help='apply edits to files instead of displaying a diff')
  parser.add_argument('-p', metavar='NUM', type=int, default=0,
                      help='strip the smallest prefix containing P slashes')
  parser.add_argument('-regex', metavar='PATTERN', default=None,
                      help='custom pattern selecting file paths to reformat '
                      '(case sensitive, overrides -iregex)')
  parser.add_argument('-iregex', metavar='PATTERN', default=
                      r'.*\.(cpp|cc|c\+\+|cxx|c|cl|h|hpp|m|mm|inc|js|ts|proto'
                      r'|protodevel|java)',
                      help='custom pattern selecting file paths to reformat '
                      '(case insensitive, overridden by -regex)')
  parser.add_argument('-sort-includes', action='store_true', default=False,
                      help='let clang-format sort include blocks')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='be more verbose, ineffective without -i')
  parser.add_argument('-style',
                      help='formatting style to apply (LLVM, Google, Chromium, '
                      'Mozilla, WebKit). Defaults to -style=file if omitted.')
  parser.add_argument('-binary', default='clang-format',
                      help='location of binary to use for clang-format')
  args = parser.parse_args()

  # Extract changed lines for each file.
  filename = None
  lines_by_file = {}

  for line in sys.stdin:
    # Match file marker lines like "+++ b/path/to/file"
    match = re.search(r'^\+\+\+\ (.*?/){%d}([^\s].*)' % args.p, line)
    if match:
      candidate = match.group(2)
      # Skip deletions/new files referencing /dev/null
      if candidate == 'dev/null' or candidate.endswith('/dev/null'):
        filename = None
      else:
        filename = candidate

    if filename is None:
      continue

    # Filter files by regex if requested
    if args.regex is not None:
      if not re.match('^%s$' % args.regex, filename):
        continue
    else:
      if not re.match('^%s$' % args.iregex, filename, re.IGNORECASE):
        continue

    # Match hunk headers and capture the added range: +<start>(,<count>)?
    match = re.search(r'^@@.*\+(\d+)(?:,(\d+))?', line)
    if match:
      start_line = int(match.group(1))
      line_count = int(match.group(2) or '1')
      if line_count == 0:
        continue
      end_line = start_line + line_count - 1
      lines_by_file.setdefault(filename, []).extend(
          ['-lines', f'{start_line}:{end_line}'])

  # Reformat files containing changes in place or show diffs.
  for filename, lines in lines_by_file.items():
    if args.i and args.verbose:
      print('Formatting {}'.format(filename))

    command = [args.binary]

    # Prefer repository configuration unless explicitly overridden.
    if args.style:
      command.extend(['-style', args.style])
    else:
      command.extend(['-style', 'file'])

    if args.sort_includes:
      command.append('-sort-includes')

    command.extend(lines)

    if args.i:
      command.append('-i')

    command.append(filename)

    p = subprocess.Popen(command,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         stdin=subprocess.PIPE,
                         universal_newlines=True)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
      if args.verbose and stderr:
        sys.stderr.write(stderr)
      sys.exit(p.returncode)

    if not args.i:
      with open(filename, 'r') as f:
        code = f.readlines()
      formatted_code = StringIO(stdout).readlines()
      diff = difflib.unified_diff(code, formatted_code,
                                  filename, filename,
                                  '(before formatting)', '(after formatting)')
      diff_string = ''.join(diff)
      if len(diff_string) > 0:
        sys.stdout.write(diff_string)


if __name__ == '__main__':
  main()