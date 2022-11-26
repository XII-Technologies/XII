# Make sure this project is built when the Editor is built
xii_add_as_runtime_dependency(ShaderCompilerDXC)

xii_add_dependency("ShaderCompiler" "ShaderCompilerDXC")