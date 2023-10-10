cmake -S ../llvm -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;cross-project-tests;mlir;polly;libc;libclc;lld;lldb;openmp;pstl;flang" -DLLVM_ENABLE_RUNTIMES="compiler-rt;libcxx;libcxxabi;libunwind" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DLLVM_TARGETS_TO_BUILD="X86" -DCLANG_INCLUDE_TESTS=ON -DLLVM_ENABLE_ASSERTIONS=ON