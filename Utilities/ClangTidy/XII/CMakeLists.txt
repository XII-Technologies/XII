set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
)

add_clang_library(clangTidyXIIModule
  XIITidyModule.cpp

  NameCheck.cpp

  LINK_LIBS
  clangTidy
  clangTidyReadabilityModule
  clangTidyUtils

  DEPENDS
  omp_gen
)

clang_target_link_libraries(clangTidyXIIModule
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangLex
)
