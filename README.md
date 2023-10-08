# cocktail language

![support](https://img.shields.io/badge/Support-CanftIn-FFD500?style=flat&labelColor=005BBB)
![version](https://img.shields.io/badge/version-0.0.1-green)
![ci](https://github.com/canftin/cocktail-lang/actions/workflows/ci.yml/badge.svg)

To be an intergrated programming language of C++. As a step by step implementing of Carbon. **In Early Time!!!**

# Build

depends on LLVM, only testing in Ubuntu yet. 

**required command:** 

`sudo apt-get install cmake g++ clang-15 bison flex libgtest-dev libgmock-dev make valgrind libbenchmark-dev llvm-15-dev libmlir-15-dev libclang-15-dev`

first:

go to thirdparty directory and execute command: python3 llvm_pull_build.py

second:

```bash
> mkdir build
> cd build
> cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/clang++-15
> make -j$(nproc)
> ctest -j$(nproc)
```

you should use clang++ to build.

# Basic grammar examples

preview in folder: [TestCase](/unittests/TestCase)

