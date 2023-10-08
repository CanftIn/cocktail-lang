import os
import subprocess
import hashlib

# 设置目标文件名和目录名
llvm_dirname = "llvm-project-3d51010a3350660160981c6b8e624dcc87c208a3"
llvm_version = "3d51010a3350660160981c6b8e624dcc87c208a3"
llvm_sha256 = "efbca707a6eb1c714b849de120309070eef282660c0f4be5b68efef62cc95cf5"
tar_filename = "llvm.tar.gz"
tar_url = "https://github.com/llvm/llvm-project/archive/{0}.tar.gz".format(llvm_version)

print(tar_url)
# 下载 LLVM 源码压缩包
# if os.path.exists(zip_filename):
#   # 创建SHA-256哈希对象
#   sha256_hash = hashlib.sha256()
# 
#   # 打开文件并逐块更新哈希对象
#   with open(zip_filename, 'rb') as file:
#     while True:
#       # 以1MB块的大小读取文件
#       data = file.read(1024 * 1024)  # 1MB
#       if not data:
#         break
#       sha256_hash.update(data)
# 
#   # 获取SHA-256哈希值的十六进制表示
#   hash_value = sha256_hash.hexdigest()
# 
#   if llvm_17_0_2_sha256 != hash_value:
#     os.remove(zip_filename)
# else:
#   os.system(f"wget {zip_url} -O {zip_filename}")

os.system(f"wget {tar_url} -O {tar_filename}")

# 解压缩源码压缩包
os.system(f"tar -xzvf {tar_filename} > /dev/null 2>&1")

# 进入 LLVM 源码目录
os.chdir(llvm_dirname)

# 创建 build 目录
os.mkdir("build")

# 进入 build 目录
os.chdir("build")

# 执行 CMake 构建
cmake_command = [
    "cmake",
    "-S", "../llvm",
    "-G", "Ninja",
    #"-DLLVM_ENABLE_PROJECTS=clang",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    "-DLLVM_TARGETS_TO_BUILD=X86",
    "-DCLANG_INCLUDE_TESTS=ON",
    "-DLLVM_ENABLE_ASSERTIONS=ON"
]
subprocess.run(cmake_command)

# 使用 Ninja 执行编译
ninja_command = ["ninja", "-j3"]
subprocess.run(ninja_command)

# 安装 LLVM
ninja_install_command = ["sudo", "ninja", "install"]
subprocess.run(ninja_install_command)