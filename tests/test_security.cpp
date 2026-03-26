/**
 * @file test_security.cpp
 * @brief Phoenix Engine Security Module Tests
 * 
 * Test suite for Phoenix Engine security features including
 * cryptography, authentication, access control, and secure data handling.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <random>

// ============================================================================
// Security Utility Functions (Example implementations for testing)
// ============================================================================

namespace phoenix {
namespace security {

/**
 * @brief Simple XOR encryption (for demonstration only)
 * @note In production, use proper cryptographic libraries
 */
std::vector<uint8_t> xorEncrypt(const std::vector<uint8_t>& data, 
                                 const std::vector<uint8_t>& key) {
    std::vector<uint8_t> result(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ key[i % key.size()];
    }
    return result;
}

/**
 * @brief Simple hash function (for demonstration only)
 * @note In production, use SHA-256 or similar
 */
uint32_t simpleHash(const std::string& input) {
    uint32_t hash = 5381;
    for (char c : input) {
        hash = ((hash << 5) + hash) + static_cast<uint8_t>(c);
    }
    return hash;
}

/**
 * @brief Secure random number generator
 */
class SecureRandom {
public:
    SecureRandom() : gen_(std::random_device{}()) {}
    
    uint32_t nextInt(uint32_t min, uint32_t max) {
        std::uniform_int_distribution<uint32_t> dist(min, max);
        return dist(gen_);
    }
    
    std::vector<uint8_t> generateBytes(size_t length) {
        std::vector<uint8_t> bytes(length);
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < length; ++i) {
            bytes[i] = static_cast<uint8_t>(dist(gen_));
        }
        return bytes;
    }
    
    std::string generateToken(size_t length = 32) {
        static const char* charset = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::string token;
        token.reserve(length);
        std::uniform_int_distribution<size_t> dist(0, strlen(charset) - 1);
        for (size_t i = 0; i < length; ++i) {
            token += charset[dist(gen_)];
        }
        return token;
    }

private:
    std::mt19937 gen_;
};

/**
 * @brief Password strength validator
 */
enum class PasswordStrength {
    Weak,
    Medium,
    Strong,
    VeryStrong
};

PasswordStrength validatePassword(const std::string& password) {
    if (password.length() < 8) {
        return PasswordStrength::Weak;
    }
    
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    
    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        else if (std::islower(c)) hasLower = true;
        else if (std::isdigit(c)) hasDigit = true;
        else hasSpecial = true;
    }
    
    int criteria = hasUpper + hasLower + hasDigit + hasSpecial;
    
    if (criteria >= 4 && password.length() >= 16) {
        return PasswordStrength::VeryStrong;
    } else if (criteria >= 3 && password.length() >= 12) {
        return PasswordStrength::Strong;
    } else if (criteria >= 2 && password.length() >= 8) {
        return PasswordStrength::Medium;
    }
    
    return PasswordStrength::Weak;
}

/**
 * @brief Access control entry
 */
struct AccessEntry {
    std::string userId;
    std::string resource;
    std::vector<std::string> permissions;
    bool isAllowed;
    
    AccessEntry(const std::string& uid, const std::string& res, 
                const std::vector<std::string>& perms, bool allowed)
        : userId(uid), resource(res), permissions(perms), isAllowed(allowed) {}
};

/**
 * @brief Simple access control manager
 */
class AccessControlManager {
public:
    void addEntry(const AccessEntry& entry) {
        entries_.push_back(entry);
    }
    
    bool checkAccess(const std::string& userId, 
                     const std::string& resource,
                     const std::string& permission) {
        for (const auto& entry : entries_) {
            if (entry.userId == userId && entry.resource == resource) {
                if (!entry.isAllowed) {
                    return false;
                }
                for (const auto& perm : entry.permissions) {
                    if (perm == permission || perm == "*") {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    void clear() {
        entries_.clear();
    }

private:
    std::vector<AccessEntry> entries_;
};

/**
 * @brief Secure buffer that zeroes memory on destruction
 */
template<typename T>
class SecureBuffer {
public:
    explicit SecureBuffer(size_t size) : data_(size, 0) {}
    
    ~SecureBuffer() {
        // Securely zero out memory
        volatile T* ptr = data_.data();
        for (size_t i = 0; i < data_.size(); ++i) {
            ptr[i] = 0;
        }
    }
    
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    size_t size() const { return data_.size(); }
    T* data() { return data_.data(); }
    const T* data() const { return data_.data(); }

private:
    std::vector<T> data_;
};

/**
 * @brief Constant-time comparison to prevent timing attacks
 */
bool constantTimeCompare(const uint8_t* a, const uint8_t* b, size_t len) {
    volatile uint8_t result = 0;
    for (size_t i = 0; i < len; ++i) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}

/**
 * @brief Token validator with expiration
 */
class TokenValidator {
public:
    std::string createToken(const std::string& userId, 
                           std::chrono::seconds validity) {
        SecureRandom rng;
        std::string token = userId + ":" + 
                           std::to_string(std::chrono::system_clock::now()
                               .time_since_epoch().count()) + ":" +
                           rng.generateToken(32);
        
        // In production, sign this token with HMAC
        uint32_t hash = simpleHash(token);
        token += ":" + std::to_string(hash);
        
        return token;
    }
    
    bool validateToken(const std::string& token, 
                      const std::string& expectedUser) {
        // Simple validation (in production, verify signature and expiration)
        return token.find(expectedUser) != std::string::npos;
    }
};

} // namespace security
} // namespace phoenix

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @test Test XOR encryption/decryption
 */
TEST(SecurityCryptoTest, XorEncryption) {
    using namespace phoenix::security;
    
    std::vector<uint8_t> plaintext = {'H', 'e', 'l', 'l', 'o'};
    std::vector<uint8_t> key = {0x42, 0x42, 0x42, 0x42, 0x42};
    
    // Encrypt
    auto ciphertext = xorEncrypt(plaintext, key);
    
    // Decrypt (XOR is symmetric)
    auto decrypted = xorEncrypt(ciphertext, key);
    
    // Verify
    EXPECT_EQ(plaintext.size(), decrypted.size());
    for (size_t i = 0; i < plaintext.size(); ++i) {
        EXPECT_EQ(plaintext[i], decrypted[i]);
    }
    
    // Verify ciphertext is different from plaintext
    bool different = false;
    for (size_t i = 0; i < plaintext.size(); ++i) {
        if (plaintext[i] != ciphertext[i]) {
            different = true;
            break;
        }
    }
    EXPECT_TRUE(different);
}

/**
 * @test Test hash function consistency
 */
TEST(SecurityCryptoTest, HashConsistency) {
    using namespace phoenix::security;
    
    std::string input = "Hello, Phoenix Engine!";
    uint32_t hash1 = simpleHash(input);
    uint32_t hash2 = simpleHash(input);
    
    // Same input should produce same hash
    EXPECT_EQ(hash1, hash2);
    
    // Different input should produce different hash
    uint32_t hash3 = simpleHash("Hello, Phoenix Engine!!");
    EXPECT_NE(hash1, hash3);
    
    // Empty string hash
    uint32_t emptyHash = simpleHash("");
    EXPECT_NE(emptyHash, hash1);
}

/**
 * @test Test secure random number generation
 */
TEST(SecurityRandomTest, GenerateBytes) {
    using namespace phoenix::security;
    
    SecureRandom rng;
    auto bytes1 = rng.generateBytes(32);
    auto bytes2 = rng.generateBytes(32);
    
    // Check length
    EXPECT_EQ(bytes1.size(), 32);
    EXPECT_EQ(bytes2.size(), 32);
    
    // Check randomness (should be different)
    EXPECT_NE(bytes1, bytes2);
    
    // Check all bytes are in valid range
    for (auto b : bytes1) {
        EXPECT_GE(static_cast<int>(b), 0);
        EXPECT_LE(static_cast<int>(b), 255);
    }
}

/**
 * @test Test secure token generation
 */
TEST(SecurityRandomTest, GenerateToken) {
    using namespace phoenix::security;
    
    SecureRandom rng;
    std::string token1 = rng.generateToken(32);
    std::string token2 = rng.generateToken(32);
    std::string tokenLong = rng.generateToken(64);
    
    // Check lengths
    EXPECT_EQ(token1.length(), 32);
    EXPECT_EQ(token2.length(), 32);
    EXPECT_EQ(tokenLong.length(), 64);
    
    // Check uniqueness
    EXPECT_NE(token1, token2);
    
    // Check charset (alphanumeric only)
    for (char c : token1) {
        EXPECT_TRUE(std::isalnum(c));
    }
}

/**
 * @test Test password strength validation
 */
TEST(SecurityPasswordTest, PasswordStrength) {
    using namespace phoenix::security;
    
    // Weak passwords
    EXPECT_EQ(validatePassword(""), PasswordStrength::Weak);
    EXPECT_EQ(validatePassword("short"), PasswordStrength::Weak);
    EXPECT_EQ(validatePassword("12345678"), PasswordStrength::Weak);
    
    // Medium passwords
    EXPECT_EQ(validatePassword("password"), PasswordStrength::Medium);
    EXPECT_EQ(validatePassword("Password1"), PasswordStrength::Medium);
    
    // Strong passwords
    EXPECT_EQ(validatePassword("Passw0rd!"), PasswordStrength::Strong);
    EXPECT_EQ(validatePassword("MyP4ssw0rd!"), PasswordStrength::Strong);
    
    // Very strong passwords
    EXPECT_EQ(validatePassword("MyV3ryStr0ng!Pass"), PasswordStrength::Strong);
    EXPECT_EQ(validatePassword("MyV3ryStr0ng!Password"), PasswordStrength::VeryStrong);
}

/**
 * @test Test access control manager
 */
TEST(SecurityAccessControlTest, CheckAccess) {
    using namespace phoenix::security;
    
    AccessControlManager acm;
    
    // Add access entries
    acm.addEntry(AccessEntry("user1", "resource1", {"read", "write"}, true));
    acm.addEntry(AccessEntry("user1", "resource2", {"read"}, true));
    acm.addEntry(AccessEntry("user2", "resource1", {}, false)); // Denied
    acm.addEntry(AccessEntry("admin", "resource1", {"*"}, true)); // Wildcard
    
    // Test allowed access
    EXPECT_TRUE(acm.checkAccess("user1", "resource1", "read"));
    EXPECT_TRUE(acm.checkAccess("user1", "resource1", "write"));
    EXPECT_FALSE(acm.checkAccess("user1", "resource1", "delete"));
    
    // Test limited access
    EXPECT_TRUE(acm.checkAccess("user1", "resource2", "read"));
    EXPECT_FALSE(acm.checkAccess("user1", "resource2", "write"));
    
    // Test denied access
    EXPECT_FALSE(acm.checkAccess("user2", "resource1", "read"));
    
    // Test wildcard permission
    EXPECT_TRUE(acm.checkAccess("admin", "resource1", "read"));
    EXPECT_TRUE(acm.checkAccess("admin", "resource1", "write"));
    EXPECT_TRUE(acm.checkAccess("admin", "resource1", "delete"));
    
    // Test non-existent user
    EXPECT_FALSE(acm.checkAccess("unknown", "resource1", "read"));
}

/**
 * @test Test secure buffer zeroing
 */
TEST(SecurityBufferTest, SecureZeroing) {
    using namespace phoenix::security;
    
    // Create buffer with sensitive data
    {
        SecureBuffer<uint8_t> buffer(16);
        for (size_t i = 0; i < buffer.size(); ++i) {
            buffer[i] = 0xFF;
        }
        
        // Verify data is set
        for (size_t i = 0; i < buffer.size(); ++i) {
            EXPECT_EQ(buffer[i], 0xFF);
        }
        
        // Buffer will be zeroed when it goes out of scope
    }
    
    // Note: We can't directly verify the zeroing after destruction,
    // but the test ensures the destructor runs without errors
}

/**
 * @test Test constant-time comparison
 */
TEST(SecurityTimingTest, ConstantTimeCompare) {
    using namespace phoenix::security;
    
    uint8_t a[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t b[] = {0x01, 0x02, 0x03, 0x04};
    uint8_t c[] = {0x01, 0x02, 0x03, 0x05};
    uint8_t d[] = {0xFF, 0x02, 0x03, 0x04};
    
    // Equal arrays
    EXPECT_TRUE(constantTimeCompare(a, b, 4));
    
    // Different last byte
    EXPECT_FALSE(constantTimeCompare(a, c, 4));
    
    // Different first byte
    EXPECT_FALSE(constantTimeCompare(a, d, 4));
    
    // Same content, different pointers
    EXPECT_TRUE(constantTimeCompare(a, a, 4));
}

/**
 * @test Test token creation and validation
 */
TEST(SecurityTokenTest, TokenValidation) {
    using namespace phoenix::security;
    
    TokenValidator validator;
    
    // Create token
    std::string token = validator.createToken("user123", std::chrono::seconds(3600));
    
    // Verify token is not empty
    EXPECT_FALSE(token.empty());
    
    // Verify token contains user ID
    EXPECT_NE(token.find("user123"), std::string::npos);
    
    // Validate token
    EXPECT_TRUE(validator.validateToken(token, "user123"));
    
    // Invalid user should fail
    EXPECT_FALSE(validator.validateToken(token, "user456"));
}

/**
 * @test Test token uniqueness
 */
TEST(SecurityTokenTest, TokenUniqueness) {
    using namespace phoenix::security;
    
    TokenValidator validator;
    
    std::string token1 = validator.createToken("user1", std::chrono::seconds(3600));
    std::string token2 = validator.createToken("user1", std::chrono::seconds(3600));
    
    // Tokens should be unique even for same user
    EXPECT_NE(token1, token2);
}

// ============================================================================
// Integration Tests
// ============================================================================

/**
 * @test Integration test: Complete authentication flow
 */
TEST(SecurityIntegrationTest, AuthenticationFlow) {
    using namespace phoenix::security;
    
    // 1. Generate secure credentials
    SecureRandom rng;
    std::string password = rng.generateToken(16);
    
    // 2. Validate password strength
    PasswordStrength strength = validatePassword(password);
    EXPECT_GE(static_cast<int>(strength), static_cast<int>(PasswordStrength::Medium));
    
    // 3. Create access control entry
    AccessControlManager acm;
    acm.addEntry(AccessEntry("testuser", "secure_resource", {"read"}, true));
    
    // 4. Create session token
    TokenValidator validator;
    std::string token = validator.createToken("testuser", std::chrono::seconds(300));
    
    // 5. Validate token
    EXPECT_TRUE(validator.validateToken(token, "testuser"));
    
    // 6. Check access
    EXPECT_TRUE(acm.checkAccess("testuser", "secure_resource", "read"));
    EXPECT_FALSE(acm.checkAccess("testuser", "secure_resource", "write"));
}

/**
 * @test Performance test: Hash function
 */
TEST(SecurityPerformanceTest, HashPerformance) {
    using namespace phoenix::security;
    
    const int iterations = 100000;
    std::string input = "Performance test input string";
    uint32_t hash = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        hash = simpleHash(input + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (< 1 second for 100k iterations)
    EXPECT_LT(duration.count(), 1000);
    
    // Ensure hash was computed
    EXPECT_GT(hash, 0);
}
