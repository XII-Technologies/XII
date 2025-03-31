# Make sure this project is built when the Editor is built
xii_add_as_runtime_dependency(ShaderCompilerDXBC)

xii_add_dependency("ShaderCompilerTool" "ShaderCompilerDXBC")
