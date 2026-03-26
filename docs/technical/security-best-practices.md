# Phoenix Engine 安全最佳实践

## 🔒 安全原则

### 1. 最小权限原则

```cpp
// ✅ 仅请求必要权限
// AndroidManifest.xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.VIBRATE" />

// ❌ 避免过度权限
<uses-permission android:name="android.permission.READ_CONTACTS" />
<uses-permission android:name="android.permission.SEND_SMS" />
```

### 2. 输入验证

```cpp
class AssetLoader {
public:
    Texture* loadTexture(const std::string& path) {
        // 验证路径
        if (!isValidPath(path)) {
            logError("Invalid path");
            return nullptr;
        }
        
        // 防止路径遍历
        if (path.find("..") != std::string::npos) {
            logError("Path traversal attempt");
            return nullptr;
        }
        
        // 验证文件扩展名
        if (!hasValidExtension(path, {".png", ".jpg", ".dds"})) {
            logError("Invalid file extension");
            return nullptr;
        }
        
        // 验证文件大小
        size_t size = getFileSize(path);
        if (size > MAX_TEXTURE_SIZE) {
            logError("File too large");
            return nullptr;
        }
        
        return loadTextureInternal(path);
    }
    
private:
    static constexpr size_t MAX_TEXTURE_SIZE = 256 * 1024 * 1024;
};
```

### 3. 资源限制

```cpp
class ResourceManager {
public:
    bool canAllocate(size_t size) {
        return currentMemory_ + size <= memoryLimit_;
    }
    
    void* allocate(size_t size) {
        if (!canAllocate(size)) {
            logWarning("Memory limit exceeded");
            evictUnusedResources();
            
            if (!canAllocate(size)) {
                return nullptr;  // 拒绝分配
            }
        }
        
        currentMemory_ += size;
        return malloc(size);
    }
    
private:
    size_t currentMemory_ = 0;
    size_t memoryLimit_ = 512 * 1024 * 1024;  // 512MB
};
```

## 🛡️ 网络安全

### 1. HTTPS 强制

```cpp
class NetworkManager {
public:
    bool download(const std::string& url) {
        // 强制使用 HTTPS
        if (!url.starts_with("https://")) {
            logError("Only HTTPS allowed");
            return false;
        }
        
        // 验证证书
        if (!validateCertificate(url)) {
            logError("Certificate validation failed");
            return false;
        }
        
        return downloadInternal(url);
    }
};
```

### 2. 数据加密

```cpp
class SecureStorage {
public:
    void save(const std::string& key, const std::string& value) {
        // 加密数据
        auto encrypted = encrypt(value, encryptionKey_);
        
        // 计算 HMAC
        auto hmac = computeHMAC(encrypted, hmacKey_);
        
        // 存储
        storage_.set(key, encrypted + hmac);
    }
    
    std::string load(const std::string& key) {
        auto data = storage_.get(key);
        
        // 验证 HMAC
        auto encrypted = data.substr(0, data.size() - HMAC_SIZE);
        auto storedHmac = data.substr(data.size() - HMAC_SIZE);
        
        if (!verifyHMAC(encrypted, storedHmac, hmacKey_)) {
            logError("HMAC verification failed");
            return "";
        }
        
        // 解密
        return decrypt(encrypted, encryptionKey_);
    }
    
private:
    std::vector<uint8_t> encryptionKey_;
    std::vector<uint8_t> hmacKey_;
    static constexpr size_t HMAC_SIZE = 32;
};
```

## 🔐 认证与授权

### 1. API 密钥管理

```cpp
class APIKeyManager {
public:
    void setKey(const std::string& key) {
        // 不在日志中记录密钥
        keyHash_ = hash(key);
        
        // 安全存储
        secureStorage_.save("api_key", key);
    }
    
    bool validateKey(const std::string& key) {
        return hash(key) == keyHash_;
    }
    
private:
    std::vector<uint8_t> keyHash_;
    SecureStorage secureStorage_;
    
    std::vector<uint8_t> hash(const std::string& input) {
        // 使用 SHA-256
        return sha256(input);
    }
};
```

### 2. 令牌管理

```cpp
class TokenManager {
public:
    std::string generateToken() {
        // 生成加密安全的随机令牌
        auto random = generateSecureRandom(32);
        return base64Encode(random);
    }
    
    bool validateToken(const std::string& token) {
        // 检查令牌是否过期
        auto it = tokens_.find(token);
        if (it == tokens_.end()) {
            return false;
        }
        
        if (std::chrono::system_clock::now() > it->second.expiry) {
            tokens_.erase(it);
            return false;
        }
        
        return true;
    }
    
private:
    struct TokenInfo {
        std::chrono::system_clock::time_point expiry;
        std::string userId;
    };
    
    std::unordered_map<std::string, TokenInfo> tokens_;
};
```

## 📁 文件安全

### 1. 安全文件访问

```cpp
class SecureFileAccess {
public:
    std::string readFile(const std::string& path) {
        // 解析路径
        fs::path fullPath = fs::weakly_canonical(baseDir_ / path);
        
        // 确保在基目录内
        if (!fullPath.string().starts_with(baseDir_.string())) {
            logError("Path traversal attempt detected");
            return "";
        }
        
        // 检查文件存在
        if (!fs::exists(fullPath)) {
            logError("File not found");
            return "";
        }
        
        // 读取文件
        std::ifstream file(fullPath);
        return std::string(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }
    
private:
    fs::path baseDir_;
};
```

### 2. 临时文件安全

```cpp
class TempFile {
public:
    TempFile() {
        // 创建安全的临时文件
        #ifdef _WIN32
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            GetTempFileNameA(tempPath, "PHX", 0, fileName_);
        #else
            strcpy(fileName_, "/tmp/phx_XXXXXX");
            int fd = mkstemp(fileName_);
            close(fd);
        #endif
    }
    
    ~TempFile() {
        // 自动清理
        if (fs::exists(fileName_)) {
            fs::remove(fileName_);
        }
    }
    
private:
    char fileName_[256];
};
```

## 🧹 内存安全

### 1. 边界检查

```cpp
class SafeArray {
public:
    float& at(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of bounds");
        }
        return data_[index];
    }
    
    float& operator[](size_t index) {
        // 调试模式检查，发布模式不检查
        #ifdef DEBUG
            if (index >= size_) {
                logError("Array index out of bounds: " + 
                         std::to_string(index));
            }
        #endif
        return data_[index];
    }
    
private:
    std::vector<float> data_;
    size_t size_;
};
```

### 2. 智能指针

```cpp
class ResourceManager {
public:
    // 使用智能指针管理资源
    std::unique_ptr<Texture> loadTexture(const std::string& path) {
        auto texture = std::make_unique<Texture>();
        
        if (!texture->load(path)) {
            return nullptr;  // 自动清理
        }
        
        return texture;
    }
    
    // 共享资源
    std::shared_ptr<Mesh> getSharedMesh(const std::string& name) {
        auto it = meshCache_.find(name);
        if (it != meshCache_.end()) {
            return it->second;
        }
        
        auto mesh = std::make_shared<Mesh>();
        mesh->load(name);
        meshCache_[name] = mesh;
        return mesh;
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshCache_;
};
```

## 🔍 日志安全

### 1. 敏感信息过滤

```cpp
class SecureLogger {
public:
    void info(const std::string& message) {
        log(filterSensitive(message), LogLevel::Info);
    }
    
    void debug(const std::string& message) {
        #ifdef DEBUG
            log(filterSensitive(message), LogLevel::Debug);
        #endif
    }
    
private:
    std::string filterSensitive(const std::string& message) {
        // 过滤密码
        std::regex passwordRegex("password[\"']?\\s*[:=]\\s*[\"']?[^\"'\\s]+");
        std::string filtered = std::regex_replace(
            message, 
            passwordRegex, 
            "password=***REDACTED***"
        );
        
        // 过滤令牌
        std::regex tokenRegex("token[\"']?\\s*[:=]\\s*[\"']?[^\"'\\s]+");
        filtered = std::regex_replace(
            filtered, 
            tokenRegex, 
            "token=***REDACTED***"
        );
        
        return filtered;
    }
};
```

### 2. 日志级别控制

```cpp
enum class LogLevel {
    Error,
    Warning,
    Info,
    Debug,
    Verbose
};

class Logger {
public:
    void setLevel(LogLevel level) {
        level_ = level;
    }
    
    void log(const std::string& message, LogLevel msgLevel) {
        if (msgLevel > level_) {
            return;  // 不记录
        }
        
        // 生产环境不记录 Debug/Verbose
        #ifndef DEBUG
            if (msgLevel >= LogLevel::Debug) {
                return;
            }
        #endif
        
        writeLog(message, msgLevel);
    }
    
private:
    LogLevel level_ = LogLevel::Info;
};
```

## 🚨 错误处理

### 1. 安全失败

```cpp
class Renderer {
public:
    bool initialize(const Config& config) {
        // 验证配置
        if (!validateConfig(config)) {
            logError("Invalid configuration");
            return false;
        }
        
        // 尝试初始化
        try {
            initializeInternal(config);
        } catch (const std::exception& e) {
            logError("Initialization failed: " + std::string(e.what()));
            
            // 清理资源
            shutdown();
            
            // 安全失败
            return false;
        }
        
        return true;
    }
    
private:
    bool validateConfig(const Config& config) {
        if (config.width == 0 || config.height == 0) {
            return false;
        }
        
        if (config.msaaSamples > 8) {
            return false;
        }
        
        return true;
    }
};
```

### 2. 断言

```cpp
#ifdef DEBUG
    #define PHX_ASSERT(cond, msg) \
        do { \
            if (!(cond)) { \
                logError("Assertion failed: " msg); \
                std::abort(); \
            } \
        } while (0)
#else
    #define PHX_ASSERT(cond, msg) ((void)0)
#endif

// 使用
void render(Mesh* mesh) {
    PHX_ASSERT(mesh != nullptr, "Mesh cannot be null");
    PHX_ASSERT(mesh->isInitialized(), "Mesh not initialized");
    
    // ...
}
```

## 📋 安全检查清单

### 开发阶段

- [ ] 所有输入都经过验证
- [ ] 使用智能指针管理内存
- [ ] 实现边界检查
- [ ] 过滤日志中的敏感信息
- [ ] 使用 HTTPS 进行网络通信
- [ ] 加密存储敏感数据

### 测试阶段

- [ ] 进行模糊测试
- [ ] 进行边界条件测试
- [ ] 进行内存泄漏测试
- [ ] 进行线程安全测试
- [ ] 进行权限测试

### 发布阶段

- [ ] 移除调试代码
- [ ] 禁用详细日志
- [ ] 启用编译器安全选项
- [ ] 进行代码审查
- [ ] 进行安全审计

---
*最后更新：2026-03-26*
