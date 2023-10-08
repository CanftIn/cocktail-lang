import hashlib

# 文件路径
file_path = 'llvm.zip'

# 创建SHA-256哈希对象
sha256_hash = hashlib.sha256()

# 打开文件并逐块更新哈希对象
with open(file_path, 'rb') as file:
    while True:
        # 以1MB块的大小读取文件
        data = file.read(1024 * 1024)  # 1MB
        if not data:
            break
        sha256_hash.update(data)

# 获取SHA-256哈希值的十六进制表示
hash_value = sha256_hash.hexdigest()

print("SHA-256 哈希值:", hash_value)