//===--- MiscTidyModule.cpp - clang-tidy ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../ClangTidy.h"

#include "../ClangTidyModule.h"

#include "../ClangTidyModuleRegistry.h"

#include "NameCheck.h"

namespace clang
{
  namespace tidy
  {
    namespace xii
    {
      class XIIModule : public ClangTidyModule
      {
      public:
        void addCheckFactories(ClangTidyCheckFactories& CheckFactories) override
        {
          CheckFactories.registerCheck<NameCheck>("xii-name-check");
        }
      };

    } // namespace xii

    // Register the MiscTidyModule using this statically initialized variable.
    static ClangTidyModuleRegistry::Add<xii::XIIModule> X("xii-module", "Adds XII specific lint checks.");

    // This anchor is used to force the linker to link in the generated object file
    // and thus register the XIIModule.
    volatile int XIIModuleAnchorSource = 0;

  } // namespace tidy
} // namespace clang
