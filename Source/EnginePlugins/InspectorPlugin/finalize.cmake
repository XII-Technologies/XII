# Copyright (c) Theophilus Eriata. All Rights Reserved.

if (TARGET Editor AND TARGET InspectorPlugin)

    # Make sure this project is built when the Editor is built
    add_dependencies(Editor InspectorPlugin)

endif()
