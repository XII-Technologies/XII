# Copyright (c) Theophilus Eriata. All Rights Reserved.

if(TARGET Editor AND TARGET EditorPluginMCP)
  add_dependencies(Editor EditorPluginMCP)
endif()

if(TARGET EditorEngineProcess AND TARGET MCPPlugin)
  add_dependencies(EditorEngineProcess MCPPlugin)
endif()
