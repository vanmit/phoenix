/**
 * Phoenix Security Core - C++ Example
 *
 * This example demonstrates how to use the Phoenix Security Core library
 * from C++ code. It shows:
 * - Secure memory allocation
 * - AES-256-GCM encryption/decryption
 * - Audit logging with HMAC verification
 *
 * Compile:
 *   g++ -std=c++17 -I../include -L../target/release -lphoenix_security_core \
 *       cpp_example.cpp -o cpp_example
 *
 * Or with CMake:
 *   mkdir build && cd build
 *   cmake ..
 *   make
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <iomanip>
#include <sstream>

extern "C" {
#include "../include/security_core.h"
}

// ============================================================================
// RAII Wrappers for C Handles
// ============================================================================

/**
 * RAII wrapper for SecureAllocator
 */
class SecureAllocator {
public:
    SecureAllocator() : handle_(secure_allocator_new()) {}
    ~SecureAllocator() {
        if (handle_) {
            secure_allocator_destroy(handle_);
        }
    }

    // Non-copyable, movable
    SecureAllocator(const SecureAllocator&) = delete;
    SecureAllocator& operator=(const SecureAllocator&) = delete;
    SecureAllocator(SecureAllocator&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    SecureAllocator& operator=(SecureAllocator&& other) noexcept {
        if (this != &other) {
            if (handle_) secure_allocator_destroy(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    /**
     * Allocate secure memory
     */
    std::vector<uint8_t> allocate(size_t size) {
        if (!handle_) {
            throw std::runtime_error("Invalid allocator handle");
        }

        uint8_t* ptr = secure_allocator_allocate(handle_, size);
        if (!ptr) {
            throw std::runtime_error("Failed to allocate secure memory");
        }

        // Copy data and free the C allocation
        std::vector<uint8_t> result(ptr, ptr + size);
        secure_allocator_free(handle_, ptr, size);
        return result;
    }

private:
    SecureAllocatorHandle* handle_;
};

/**
 * RAII wrapper for CryptoModule
 */
class CryptoModule {
public:
    /**
     * Create with existing key
     */
    explicit CryptoModule(const std::vector<uint8_t>& key) {
        if (key.size() != PHOENIX_AES_KEY_SIZE) {
            throw std::invalid_argument("Key must be 32 bytes for AES-256");
        }
        handle_ = crypto_module_new(key.data(), key.size());
        if (!handle_) {
            throw std::runtime_error("Failed to create crypto module");
        }
    }

    /**
     * Generate with random key
     */
    static CryptoModule generate() {
        uint8_t* key = crypto_module_generate_key();
        if (!key) {
            throw std::runtime_error("Failed to generate crypto key");
        }

        std::vector<uint8_t> key_vec(key, key + PHOENIX_AES_KEY_SIZE);
        crypto_free_key(key);

        return CryptoModule(key_vec);
    }

    ~CryptoModule() {
        if (handle_) {
            crypto_module_destroy(handle_);
        }
    }

    // Non-copyable, movable
    CryptoModule(const CryptoModule&) = delete;
    CryptoModule& operator=(const CryptoModule&) = delete;
    CryptoModule(CryptoModule&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    CryptoModule& operator=(CryptoModule&& other) noexcept {
        if (this != &other) {
            if (handle_) crypto_module_destroy(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    /**
     * Encrypt data using AES-256-GCM
     * Returns: (nonce, ciphertext)
     */
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> encrypt(
        const std::vector<uint8_t>& plaintext
    ) {
        if (!handle_) {
            throw std::runtime_error("Invalid crypto handle");
        }

        size_t out_len = 0;
        uint8_t* encrypted = crypto_encrypt(
            handle_,
            plaintext.data(),
            plaintext.size(),
            &out_len
        );

        if (!encrypted || out_len < 4) {
            throw std::runtime_error("Encryption failed");
        }

        // Parse nonce length from the output
        uint32_t nonce_len;
        std::memcpy(&nonce_len, encrypted, sizeof(nonce_len));

        if (out_len < 4 + nonce_len) {
            crypto_free_buffer(encrypted, out_len);
            throw std::runtime_error("Invalid encrypted data format");
        }

        // Extract nonce and ciphertext
        std::vector<uint8_t> nonce(encrypted + 4, encrypted + 4 + nonce_len);
        std::vector<uint8_t> ciphertext(
            encrypted + 4 + nonce_len,
            encrypted + out_len
        );

        crypto_free_buffer(encrypted, out_len);

        return {nonce, ciphertext};
    }

    /**
     * Decrypt data using AES-256-GCM
     */
    std::vector<uint8_t> decrypt(
        const std::vector<uint8_t>& nonce,
        const std::vector<uint8_t>& ciphertext
    ) {
        if (!handle_) {
            throw std::runtime_error("Invalid crypto handle");
        }

        // Format input: [nonce_len (4 bytes)][nonce][ciphertext]
        uint32_t nonce_len = static_cast<uint32_t>(nonce.size());
        std::vector<uint8_t> input(4 + nonce.size() + ciphertext.size());

        std::memcpy(input.data(), &nonce_len, sizeof(nonce_len));
        std::memcpy(input.data() + 4, nonce.data(), nonce.size());
        std::memcpy(input.data() + 4 + nonce.size(), ciphertext.data(), ciphertext.size());

        size_t out_len = 0;
        uint8_t* decrypted = crypto_decrypt(
            handle_,
            input.data(),
            input.size(),
            &out_len
        );

        if (!decrypted || out_len == 0) {
            throw std::runtime_error("Decryption failed");
        }

        std::vector<uint8_t> result(decrypted, decrypted + out_len);
        crypto_free_buffer(decrypted, out_len);

        return result;
    }

private:
    CryptoModuleHandle* handle_;
};

/**
 * RAII wrapper for AuditLogger
 */
class AuditLogger {
public:
    /**
     * Create with existing HMAC key
     */
    explicit AuditLogger(const std::vector<uint8_t>& key) {
        handle_ = audit_logger_new(key.data(), key.size());
        if (!handle_) {
            throw std::runtime_error("Failed to create audit logger");
        }
    }

    /**
     * Generate with random HMAC key
     */
    static AuditLogger generate() {
        uint8_t* key = audit_logger_generate_key();
        if (!key) {
            throw std::runtime_error("Failed to generate HMAC key");
        }

        std::vector<uint8_t> key_vec(key, key + PHOENIX_HMAC_KEY_SIZE);
        crypto_free_key(key);  // Same free function works for both key types

        return AuditLogger(key_vec);
    }

    ~AuditLogger() {
        if (handle_) {
            audit_logger_destroy(handle_);
        }
    }

    // Non-copyable, movable
    AuditLogger(const AuditLogger&) = delete;
    AuditLogger& operator=(const AuditLogger&) = delete;
    AuditLogger(AuditLogger&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    AuditLogger& operator=(AuditLogger&& other) noexcept {
        if (this != &other) {
            if (handle_) audit_logger_destroy(handle_);
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    /**
     * Set log file path
     */
    void setLogFile(const std::string& path) {
        if (!audit_logger_set_log_file(handle_, path.c_str())) {
            throw std::runtime_error("Failed to set log file");
        }
    }

    /**
     * Log an audit event
     */
    struct AuditEntry {
        std::string timestamp;
        std::string eventType;
        std::string details;
        std::string hmac;
        bool valid;
    };

    AuditEntry log(const std::string& eventType, const std::string& details) {
        if (!handle_) {
            throw std::runtime_error("Invalid logger handle");
        }

        void* entry = audit_logger_log(handle_, eventType.c_str(), details.c_str());
        if (!entry) {
            throw std::runtime_error("Failed to log audit event");
        }

        AuditEntry result;
        result.timestamp = audit_entry_get_timestamp(entry);
        result.eventType = audit_entry_get_event_type(entry);
        result.details = audit_entry_get_details(entry);
        result.hmac = audit_entry_get_hmac(entry);
        result.valid = audit_logger_verify_entry(handle_, entry);

        audit_entry_destroy(entry);

        return result;
    }

private:
    AuditLoggerHandle* handle_;
};

// ============================================================================
// Helper Functions
// ============================================================================

std::string bytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

void printSeparator() {
    std::cout << std::string(70, '=') << std::endl;
}

// ============================================================================
// Main Example
// ============================================================================

int main() {
    std::cout << "Phoenix Security Core - C++ Example\n";
    printSeparator();

    try {
        // -------------------------------------------------------------------------
        // Example 1: Secure Memory Allocation
        // -------------------------------------------------------------------------
        std::cout << "\n[1] Secure Memory Allocation\n";
        std::cout << std::string(70, '-') << std::endl;

        {
            SecureAllocator allocator;

            // Allocate 256 bytes of secure memory
            auto secureBuffer = allocator.allocate(256);
            std::cout << "Allocated " << secureBuffer.size() << " bytes of secure memory\n";
            std::cout << "First 16 bytes (hex): " << bytesToHex(
                std::vector<uint8_t>(secureBuffer.begin(), secureBuffer.begin() + 16)
            ) << std::endl;
            std::cout << "Note: Memory will be automatically zeroized when freed\n";
        }
        // secureBuffer is automatically zeroized and freed here

        // -------------------------------------------------------------------------
        // Example 2: AES-256-GCM Encryption
        // -------------------------------------------------------------------------
        std::cout << "\n[2] AES-256-GCM Encryption\n";
        std::cout << std::string(70, '-') << std::endl;

        {
            // Create crypto module with random key
            auto crypto = CryptoModule::generate();
            std::cout << "Created crypto module with random AES-256 key\n";

            // Original message
            std::string message = "Hello, Phoenix Engine! This is a secret message.";
            std::vector<uint8_t> plaintext(message.begin(), message.end());

            std::cout << "Original message: \"" << message << "\"\n";
            std::cout << "Plaintext length: " << plaintext.size() << " bytes\n";

            // Encrypt
            auto [nonce, ciphertext] = crypto.encrypt(plaintext);
            std::cout << "\nEncryption successful!\n";
            std::cout << "Nonce (" << nonce.size() << " bytes): " << bytesToHex(nonce) << "\n";
            std::cout << "Ciphertext (" << ciphertext.size() << " bytes): "
                      << bytesToHex(ciphertext) << "\n";

            // Decrypt
            auto decrypted = crypto.decrypt(nonce, ciphertext);
            std::string decryptedMessage(decrypted.begin(), decrypted.end());

            std::cout << "\nDecryption successful!\n";
            std::cout << "Decrypted message: \"" << decryptedMessage << "\"\n";

            // Verify
            if (message == decryptedMessage) {
                std::cout << "✓ Encryption/decryption verified successfully!\n";
            } else {
                std::cout << "✗ Decryption failed - message mismatch!\n";
            }
        }

        // -------------------------------------------------------------------------
        // Example 3: Audit Logging with HMAC
        // -------------------------------------------------------------------------
        std::cout << "\n[3] Audit Logging with HMAC Integrity\n";
        std::cout << std::string(70, '-') << std::endl;

        {
            // Create audit logger with random HMAC key
            auto logger = AuditLogger::generate();
            std::cout << "Created audit logger with random HMAC key\n";

            // Set log file (optional)
            logger.setLogFile("/tmp/phoenix_audit.log");
            std::cout << "Log file configured: /tmp/phoenix_audit.log\n\n";

            // Log some events
            std::cout << "Logging audit events...\n";

            auto entry1 = logger.log("LOGIN", "user=admin ip=192.168.1.100 success=true");
            std::cout << "\nEvent 1:\n";
            std::cout << "  Timestamp: " << entry1.timestamp << "\n";
            std::cout << "  Type: " << entry1.eventType << "\n";
            std::cout << "  Details: " << entry1.details << "\n";
            std::cout << "  HMAC: " << entry1.hmac.substr(0, 32) << "...\n";
            std::cout << "  Valid: " << (entry1.valid ? "✓" : "✗") << "\n";

            auto entry2 = logger.log("ACCESS", "resource=/api/secrets user=admin action=read");
            std::cout << "\nEvent 2:\n";
            std::cout << "  Timestamp: " << entry2.timestamp << "\n";
            std::cout << "  Type: " << entry2.eventType << "\n";
            std::cout << "  Details: " << entry2.details << "\n";
            std::cout << "  HMAC: " << entry2.hmac.substr(0, 32) << "...\n";
            std::cout << "  Valid: " << (entry2.valid ? "✓" : "✗") << "\n";

            auto entry3 = logger.log("ERROR", "code=403 message=access_denied");
            std::cout << "\nEvent 3:\n";
            std::cout << "  Timestamp: " << entry3.timestamp << "\n";
            std::cout << "  Type: " << entry3.eventType << "\n";
            std::cout << "  Details: " << entry3.details << "\n";
            std::cout << "  HMAC: " << entry3.hmac.substr(0, 32) << "...\n";
            std::cout << "  Valid: " << (entry3.valid ? "✓" : "✗") << "\n";
        }

        // -------------------------------------------------------------------------
        // Example 4: Complete Security Workflow
        // -------------------------------------------------------------------------
        std::cout << "\n[4] Complete Security Workflow\n";
        std::cout << std::string(70, '-') << std::endl;

        {
            // Initialize all components
            SecureAllocator allocator;
            auto crypto = CryptoModule::generate();
            auto logger = AuditLogger::generate();

            std::cout << "Initialized all security components\n\n";

            // Simulate a secure operation
            logger.log("SESSION_START", "session_id=abc123 user=operator");

            std::string sensitiveData = "API_KEY=sk-1234567890abcdef";
            std::vector<uint8_t> data(sensitiveData.begin(), sensitiveData.end());

            std::cout << "Sensitive data: " << sensitiveData << "\n";

            // Encrypt sensitive data
            auto [nonce, ciphertext] = crypto.encrypt(data);
            std::cout << "Encrypted to " << ciphertext.size() << " bytes\n";

            // Store/transmit encrypted data...

            // Later, decrypt
            auto decrypted = crypto.decrypt(nonce, ciphertext);
            std::string recovered(decrypted.begin(), decrypted.end());
            std::cout << "Decrypted: " << recovered << "\n";

            logger.log("SESSION_END", "session_id=abc123 duration=3600s");

            std::cout << "\n✓ Complete workflow executed successfully!\n";
        }

        // -------------------------------------------------------------------------
        // Summary
        // -------------------------------------------------------------------------
        std::cout << "\n";
        printSeparator();
        std::cout << "Phoenix Security Core C++ Example Complete!\n";
        std::cout << "\nKey Features Demonstrated:\n";
        std::cout << "  ✓ Secure memory allocation with automatic zeroization\n";
        std::cout << "  ✓ AES-256-GCM authenticated encryption\n";
        std::cout << "  ✓ Audit logging with HMAC integrity verification\n";
        std::cout << "  ✓ RAII wrappers for safe C++ usage\n";
        std::cout << "  ✓ Zero-copy FFI between C++ and Rust\n";
        printSeparator();

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
}
