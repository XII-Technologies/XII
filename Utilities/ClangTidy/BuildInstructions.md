# Building XII Custom Version Of Clang-Tidy

All commands given should be executed in a powershell.

* Clone the LLVM Repository: `git clone https://github.com/llvm/llvm-project`
* `cd llvm-project`
* Checkout the latest release version (for the current build llvm-15.0.3 is used): `git checkout llvmorg-15.0.3`
* Apply the `llvm-xii` patch: `git apply llvm-xii.patch`
* Copy the XII folder to `llvm-project/clang-tools-extra/clang-tidy/`
* Create a build folder: `mkdir build`
* `cd build`
* Run CMake: `cmake -B . -S ..\llvm -DLLVM_ENABLE_PROJECTS="clang-tools-extra;clang" -DLLVM_TARGETS_TO_BUILD=X86`
* Open the generated solution and build the clang-tidy executable in Release. Copy the resulting executable to `XII/Data/Tools/Precompiled/clang-tidy/clang-tidy.exe` when done

## Known Issues

* When running `clang-tidy.exe` out of the directory it was built into, it will pick up a different set of header files and compile errors might appear. So copy the exectable out of the build directory before attempting to run it on the XII source code. For local minimal test cases the `clang-tidy.exe` can remain in the build output directory.
