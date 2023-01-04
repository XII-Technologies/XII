
if(XII_BUILD_DILIGENT)

if (TARGET spirv-cross-core)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(spirv-cross-core)
    endif()
endif()


if (TARGET SPIRV-Tools-opt)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(SPIRV-Tools-opt)
    endif()
endif()


if (TARGET SPIRV)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(SPIRV)
    endif()
endif()


if (TARGET glslang)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(glslang)
    endif()
endif()


if (TARGET SPIRV-Tools-static)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(SPIRV-Tools-static)
    endif()
endif()


if (TARGET volk_headers)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(volk_headers)
    endif()
endif()


if (TARGET xxhash)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(xxhash)
    endif()
endif()


if (TARGET MachineIndependent)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(MachineIndependent)
    endif()
endif()


if (TARGET OGLCompiler)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(OGLCompiler)
    endif()
endif()


if (TARGET OSDependent)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(OSDependent)
    endif()
endif()


if (TARGET GenericCodeGen)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(GenericCodeGen)
    endif()
endif()

if (TARGET Vulkan-Headers)
    if(COMMAND custom_post_configure_target)
        custom_post_configure_target(Vulkan-Headers)
    endif()
endif()

endif()
