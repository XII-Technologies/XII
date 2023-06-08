@powershell -NoProfile -ExecutionPolicy ByPass python ./ClangFormat/RunClangFormat.py ^
--clang-format-binary="..\Source\BuildTools\FormatValidation\clang-format_10.0.0.exe" ^
..\Source\BuildSystem ^
..\Source\BuildTools ^
..\Source\Editor ^
..\Source\EditorPlugins ^
..\Source\Engine ^
..\Source\EnginePlugins ^
..\Source\Samples ^
..\Source\Tools ^
..\Source\UnitTests
