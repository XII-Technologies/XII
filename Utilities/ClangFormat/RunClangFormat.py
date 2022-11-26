
#!/usr/bin/env python

# RunClangFormat
# Script to run clang-format on all files in a directory
# 
# Usage
# To run clang-format over all files in source_dir directory, just run:
# 
# python RunClangFormat.py source_dir
# If you want to specify the style for formatting (by default file style is used), then execute:
# 
# python RunClangFormat.py -style FILE source_dir
# You can specify the number of threads that will be used to reformat the code with -j option (when unspecified, the CPU count will be used):
# 
# python RunClangFormat.py -j 16 source_dir
# You may specify multiple directories to run clang-format over:
# 
# python RunClangFormat.py source_dir1 source_dir2


import ClangFormat

ClangFormat.main()
