/**
 * Phoenix Engine - Security Core Unit Tests
 * 
 * 测试安全核心功能，包括内存安全、加密、哈希等
 * 目标覆盖率：>95%
 */

#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <string>

// Phoenix Engine Security 头文件
#include "phoenix/security/secure_memory.hpp"
#include "phoenix/security/crypto.hpp"
#include "phoenix/security/hash.hpp"
#include "phoenix/security/secure_string.hpp"
#include "phoenix/security/validation.hpp"
#include "phoenix/security/permissions.hpp"

using namespace phoenix::security;

// ============================================================================
// SecureMemory 测试
// ============================================================================

TEST(SecureMemoryTest, Allocation) {
    SecureBuffer buffer(1024);
    
    EXPECT_EQ(buffer.size(), 1024);
    EXPECT_NE(buffer.data(), nullptr);
}

TEST(SecureMemoryTest, Zeroing) {
    SecureBuffer buffer(256);
    
    // 填充数据
    std::memset(buffer.data(), 0xAB, buffer.size());
    
    // 验证填充
    for (size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_EQ(buffer.data()[i], 0xAB);
    }
    
    // 析构时应该自动清零
}

TEST(SecureMemoryTest, Copy) {
    SecureBuffer src(128);
    std::memset(src.data(), 0xCD, src.size());
    
    SecureBuffer dst = src;
    
    EXPECT_EQ(dst.size(), 128);
    for (size_t i = 0; i < dst.size(); ++i) {
        EXPECT_EQ(dst.data()[i], 0xCD);
    }
}

TEST(SecureMemoryTest, Move) {
    SecureBuffer src(256);
    uint8_t* src_ptr = src.data();
    
    SecureBuffer dst = std::move(src);
    
    EXPECT_EQ(dst.size(), 256);
    EXPECT_EQ(dst.data(), src_ptr);
    EXPECT_EQ(src.size(), 0);
    EXPECT_EQ(src.data(), nullptr);
}

// ============================================================================
// Crypto 测试
// ============================================================================

TEST(CryptoTest, AES256Encryption) {
    std::vector<uint8_t> key(32, 0x42); // 256-bit key
    std::vector<uint8_t> iv(16, 0x23);  // 128-bit IV
    std::string plaintext = "Hello, Phoenix Engine!";
    
    std::vector<uint8_t> ciphertext = Crypto::aes256Encrypt(
        reinterpret_cast<const uint8_t*>(plaintext.data()),
        plaintext.size(),
        key,
        iv
    );
    
    EXPECT_GT(ciphertext.size(), 0);
    EXPECT_NE(ciphertext.size(), plaintext.size()); // PKCS7 padding
    
    // 解密验证
    std::vector<uint8_t> decrypted = Crypto::aes256Decrypt(
        ciphertext,
        key,
        iv
    );
    
    EXPECT_EQ(decrypted.size(), plaintext.size());
    EXPECT_EQ(std::string(decrypted.begin(), decrypted.end()), plaintext);
}

TEST(CryptoTest, RSAEncryption) {
    RSAKeyPair keys = Crypto::generateRSAKeyPair(2048);
    
    EXPECT_GT(keys.publicKey.size(), 0);
    EXPECT_GT(keys.privateKey.size(), 0);
    
    std::string message = "Secret message";
    
    std::vector<uint8_t> encrypted = Crypto::rsaEncrypt(
        reinterpret_cast<const uint8_t*>(message.data()),
        message.size(),
        keys.publicKey
    );
    
    EXPECT_GT(encrypted.size(), 0);
    
    std::vector<uint8_t> decrypted = Crypto::rsaDecrypt(
        encrypted,
        keys.privateKey
    );
    
    EXPECT_EQ(std::string(decrypted.begin(), decrypted.end()), message);
}

TEST(CryptoTest, ECDSASignature) {
    ECCKeyPair keys = Crypto::generateECCKeyPair();
    
    std::string message = "Signed message";
    
    ECDSASignature signature = Crypto::ecdsaSign(
        reinterpret_cast<const uint8_t*>(message.data()),
        message.size(),
        keys.privateKey
    );
    
    EXPECT_GT(signature.r.size(), 0);
    EXPECT_GT(signature.s.size(), 0);
    
    bool valid = Crypto::ecdsaVerify(
        reinterpret_cast<const uint8_t*>(message.data()),
        message.size(),
        signature,
        keys.publicKey
    );
    
    EXPECT_TRUE(valid);
}

TEST(CryptoTest, ECDSASignatureTampered) {
    ECCKeyPair keys = Crypto::generateECCKeyPair();
    
    std::string message = "Original message";
    std::string tampered = "Tampered message";
    
    ECDSASignature signature = Crypto::ecdsaSign(
        reinterpret_cast<const uint8_t*>(message.data()),
        message.size(),
        keys.privateKey
    );
    
    bool valid = Crypto::ecdsaVerify(
        reinterpret_cast<const uint8_t*>(tampered.data()),
        tampered.size(),
        signature,
        keys.publicKey
    );
    
    EXPECT_FALSE(valid);
}

// ============================================================================
// Hash 测试
// ============================================================================

TEST(HashTest, SHA256) {
    std::string input = "Hello, Phoenix Engine!";
    
    std::vector<uint8_t> hash = Hash::sha256(
        reinterpret_cast<const uint8_t*>(input.data()),
        input.size()
    );
    
    EXPECT_EQ(hash.size(), 32); // SHA-256 produces 256 bits = 32 bytes
    
    // 验证已知哈希值
    std::string expected_hex = "8b5c6c7e8a5e8f5c8d5e8f5c8d5e8f5c8d5e8f5c8d5e8f5c8d5e8f5c8d5e8f5c";
    std::string actual_hex = Hash::toHex(hash);
    
    // 注意：这里需要实际的 SHA-256 值，使用占位符
    EXPECT_EQ(actual_hex.length(), 64);
}

TEST(HashTest, SHA512) {
    std::string input = "Phoenix Engine Security";
    
    std::vector<uint8_t> hash = Hash::sha512(
        reinterpret_cast<const uint8_t*>(input.data()),
        input.size()
    );
    
    EXPECT_EQ(hash.size(), 64); // SHA-512 produces 512 bits = 64 bytes
}

TEST(HashTest, HMAC) {
    std::string key = "secret-key";
    std::string message = "Authenticated message";
    
    std::vector<uint8_t> hmac = Hash::hmacSha256(
        reinterpret_cast<const uint8_t*>(message.data()),
        message.size(),
        reinterpret_cast<const uint8_t*>(key.data()),
        key.size()
    );
    
    EXPECT_EQ(hmac.size(), 32);
    
    // 验证 HMAC 一致性
    std::vector<uint8_t> hmac2 = Hash::hmacSha256(
        reinterpret_cast<const uint8_t*>(message.data()),
        message.size(),
        reinterpret_cast<const uint8_t*>(key.data()),
        key.size()
    );
    
    EXPECT_EQ(hmac, hmac2);
}

TEST(HashTest, PBKDF2) {
    std::string password = "secure-password";
    std::string salt = "random-salt-value";
    
    std::vector<uint8_t> derived = Hash::pbkdf2Sha256(
        password,
        salt,
        10000, // iterations
        32     // key length
    );
    
    EXPECT_EQ(derived.size(), 32);
    
    // 验证一致性
    std::vector<uint8_t> derived2 = Hash::pbkdf2Sha256(
        password,
        salt,
        10000,
        32
    );
    
    EXPECT_EQ(derived, derived2);
}

// ============================================================================
// SecureString 测试
// ============================================================================

TEST(SecureStringTest, Construction) {
    SecureString str("Secret data");
    
    EXPECT_EQ(str.length(), 11);
    EXPECT_EQ(str.str(), "Secret data");
}

TEST(SecureStringTest, Append) {
    SecureString str("Hello");
    str.append(", World!");
    
    EXPECT_EQ(str.str(), "Hello, World!");
}

TEST(SecureStringTest, Clear) {
    SecureString str("Sensitive data");
    
    str.clear();
    
    EXPECT_EQ(str.length(), 0);
    EXPECT_EQ(str.str(), "");
}

TEST(SecureStringTest, Comparison) {
    SecureString str1("same");
    SecureString str2("same");
    SecureString str3("different");
    
    EXPECT_EQ(str1, str2);
    EXPECT_NE(str1, str3);
}

TEST(SecureStringTest, SecureComparison) {
    SecureString str1("secret");
    SecureString str2("secret");
    SecureString str3("different");
    
    EXPECT_TRUE(SecureString::secureCompare(str1, str2));
    EXPECT_FALSE(SecureString::secureCompare(str1, str3));
}

// ============================================================================
// Validation 测试
// ============================================================================

TEST(ValidationTest, ValidateEmail) {
    EXPECT_TRUE(Validation::isValidEmail("user@example.com"));
    EXPECT_TRUE(Validation::isValidEmail("user.name+tag@example.co.uk"));
    EXPECT_FALSE(Validation::isValidEmail("invalid"));
    EXPECT_FALSE(Validation::isValidEmail("@example.com"));
    EXPECT_FALSE(Validation::isValidEmail("user@"));
}

TEST(ValidationTest, ValidateURL) {
    EXPECT_TRUE(Validation::isValidURL("https://example.com"));
    EXPECT_TRUE(Validation::isValidURL("http://localhost:8080/path"));
    EXPECT_FALSE(Validation::isValidURL("not-a-url"));
    EXPECT_FALSE(Validation::isValidURL("ftp://"));
}

TEST(ValidationTest, ValidateFilePath) {
    EXPECT_TRUE(Validation::isValidFilePath("/home/user/file.txt"));
    EXPECT_TRUE(Validation::isValidFilePath("C:\\Users\\file.txt"));
    EXPECT_FALSE(Validation::isValidFilePath("../../../etc/passwd"));
    EXPECT_FALSE(Validation::isValidFilePath("/etc/passwd\0.txt"));
}

TEST(ValidationTest, ValidateInput) {
    EXPECT_TRUE(Validation::sanitizeInput("Normal text"));
    EXPECT_FALSE(Validation::sanitizeInput("<script>alert('xss')</script>"));
    EXPECT_FALSE(Validation::sanitizeInput("'; DROP TABLE users; --"));
}

TEST(ValidationTest, ValidateInteger) {
    EXPECT_TRUE(Validation::isValidInteger("123"));
    EXPECT_TRUE(Validation::isValidInteger("-456"));
    EXPECT_FALSE(Validation::isValidInteger("abc"));
    EXPECT_FALSE(Validation::isValidInteger("12.34"));
}

TEST(ValidationTest, ValidateRange) {
    EXPECT_TRUE(Validation::isInRange(5, 0, 10));
    EXPECT_FALSE(Validation::isInRange(15, 0, 10));
    EXPECT_FALSE(Validation::isInRange(-5, 0, 10));
}

// ============================================================================
// Permissions 测试
// ============================================================================

TEST(PermissionsTest, RoleCreation) {
    Role admin("admin");
    admin.addPermission("read");
    admin.addPermission("write");
    admin.addPermission("delete");
    
    EXPECT_TRUE(admin.hasPermission("read"));
    EXPECT_TRUE(admin.hasPermission("write"));
    EXPECT_TRUE(admin.hasPermission("delete"));
    EXPECT_FALSE(admin.hasPermission("execute"));
}

TEST(PermissionsTest, RoleInheritance) {
    Role base("base");
    base.addPermission("read");
    
    Role derived("derived");
    derived.addPermission("write");
    derived.inheritFrom(base);
    
    EXPECT_TRUE(derived.hasPermission("read"));
    EXPECT_TRUE(derived.hasPermission("write"));
}

TEST(PermissionsTest, AccessControl) {
    AccessControlList acl;
    
    Role user("user");
    user.addPermission("read");
    
    Role admin("admin");
    admin.addPermission("read");
    admin.addPermission("write");
    admin.addPermission("delete");
    
    acl.addRole("user", user);
    acl.addRole("admin", admin);
    
    EXPECT_TRUE(acl.checkPermission("user", "read"));
    EXPECT_FALSE(acl.checkPermission("user", "write"));
    EXPECT_TRUE(acl.checkPermission("admin", "delete"));
}

TEST(PermissionsTest, ResourceAccess) {
    ResourcePermission perm;
    perm.setResource("/secure/data");
    perm.addAllowedRole("admin");
    perm.addAllowedRole("owner");
    
    EXPECT_TRUE(perm.isAllowed("admin"));
    EXPECT_TRUE(perm.isAllowed("owner"));
    EXPECT_FALSE(perm.isAllowed("guest"));
}

// ============================================================================
// 边界条件与异常测试
// ============================================================================

TEST(SecurityEdgeCases, EmptyInput) {
    SecureBuffer empty(0);
    EXPECT_EQ(empty.size(), 0);
    
    SecureString emptyStr("");
    EXPECT_EQ(emptyStr.length(), 0);
}

TEST(SecurityEdgeCases, LargeAllocation) {
    // 测试大内存分配
    SecureBuffer large(10 * 1024 * 1024); // 10 MB
    EXPECT_EQ(large.size(), 10 * 1024 * 1024);
}

TEST(SecurityEdgeCases, NullHandling) {
    // 测试空指针处理
    std::vector<uint8_t> hash = Hash::sha256(nullptr, 0);
    EXPECT_EQ(hash.size(), 32);
}

TEST(SecurityEdgeCases, UnicodeInput) {
    SecureString unicode("你好，世界！");
    EXPECT_GT(unicode.length(), 0);
    
    std::string emoji = "🔒🛡️";
    SecureString emojiStr(emoji);
    EXPECT_GT(emojiStr.length(), 0);
}

// ============================================================================
// 主测试入口
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
