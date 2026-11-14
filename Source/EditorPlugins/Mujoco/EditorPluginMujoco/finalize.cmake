# Copyright (c) Theophilus Eriata. All Rights Reserved.

if (TARGET Editor AND TARGET EditorPluginMujoco)

  # Ensure this project is built when the Editor is built.
  add_dependencies(Editor EditorPluginMujoco)

endif()
