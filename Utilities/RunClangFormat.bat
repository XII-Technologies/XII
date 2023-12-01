@powershell -NoProfile -ExecutionPolicy ByPass python %~dp0\ClangFormat\RunClangFormat.py ^
--clang-format-binary="%~dp0..\Source\BuildTools\FormatValidation\clang-format_10.0.0.exe" ^
%~dp0..\Source\BuildSystem ^
%~dp0..\Source\BuildTools ^
%~dp0..\Source\Editor ^
%~dp0..\Source\EditorPlugins ^
%~dp0..\Source\Engine ^
%~dp0..\Source\EnginePlugins ^
%~dp0..\Source\Samples ^
%~dp0..\Source\Tools ^
%~dp0..\Source\UnitTests ^
%~dp0..\Source\Data\Base\Shaders
