if (TARGET Editor AND TARGET EditorPluginVisualScript)

    # Ensure this project is built when the Editor is built
    add_dependencies(Editor EditorPluginVisualScript)

endif()
