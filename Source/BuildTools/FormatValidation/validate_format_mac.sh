#!/bin/bash
python3 clang-format-validate.py --clang-format-executable ./clang-format_mac_10.0.0 \
-r validate_format ../../../Source/Editor ../../../Source/EditorPlugins ../../../Source/Engine ../../../Source/EnginePlugins ../../../Source/Samples ../../../Source/Tools ../../../Source/UnitTests \
