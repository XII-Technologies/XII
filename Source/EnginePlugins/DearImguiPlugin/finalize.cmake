# Copyright (c) Theophilus Eriata. All Rights Reserved.

if (TARGET Editor AND TARGET DearImguiPlugin)

  # Ensure this project is built when the Editor is built.
  add_dependencies(Editor DearImguiPlugin)

endif()
