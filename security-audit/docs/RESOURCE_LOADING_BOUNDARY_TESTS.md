# Resource Loading Boundary Testing
# Security tests for asset loading with boundary checks

## Overview

Resource loading is a critical attack surface. This document defines tests
for validating that all resource loading operations properly enforce:
- Path boundaries (no path traversal)
- Size limits (no DoS via large files)
- Type validation (no arbitrary file reads)
- Symlink handling (no symlink attacks)

## Test Matrix

| Test Category | Tests | Status |
|--------------|-------|--------|
| Path Traversal | 8 | ✅ Complete |
| Size Limits | 4 | ✅ Complete |
| Symlink Attacks | 4 | ✅ Complete |
| Type Validation | 6 | ✅ Complete |
| Race Conditions | 3 | ⏳ Pending |
| Unicode Attacks | 4 | ⏳ Pending |

## Test Cases

### 1. Path Traversal Tests

```cpp
TEST(ResourceBoundaryTest, BasicPathTraversal) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Basic traversal
    EXPECT_FALSE(rm.loadFile("../etc/passwd").valid());
    EXPECT_FALSE(rm.loadFile("../../etc/passwd").valid());
    EXPECT_FALSE(rm.loadFile("../../../etc/passwd").valid());
}

TEST(ResourceBoundaryTest, EncodedPathTraversal) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // URL-encoded traversal
    EXPECT_FALSE(rm.loadFile("..%2Fetc%2Fpasswd").valid());
    EXPECT_FALSE(rm.loadFile("..%252Fetc%252Fpasswd").valid());  // Double encoding
}

TEST(ResourceBoundaryTest, MixedSeparatorTraversal) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Mixed separators (Windows-style on Unix)
    EXPECT_FALSE(rm.loadFile("..\\..\\etc\\passwd").valid());
}

TEST(ResourceBoundaryTest, NullByteInjection) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Null byte injection
    std::string path = "/assets/valid.png";
    path.push_back('\0');
    path.append("/etc/passwd");
    
    EXPECT_FALSE(rm.loadFile(path).valid());
}

TEST(ResourceBoundaryTest, AbsolutePathRejection) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Absolute paths should be rejected or resolved relative to root
    EXPECT_FALSE(rm.loadFile("/etc/passwd").valid());
    EXPECT_FALSE(rm.loadFile("/assets/../etc/passwd").valid());
}

TEST(ResourceBoundaryTest, SymbolicLinkInPath) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Path containing symlink component
    // (assuming /assets/link -> /etc exists)
    EXPECT_FALSE(rm.loadFile("link/passwd").valid());
}

TEST(ResourceBoundaryTest, UnicodeNormalization) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Unicode normalization attack (NFC vs NFD)
    // "assets" with combining characters that normalize to traversal
    EXPECT_FALSE(rm.loadFile("asse\u0300ts/../etc/passwd").valid());
}

TEST(ResourceBoundaryTest, LongPathAttack) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    rm.setMaxPathLength(4096);
    
    // Very long path
    std::string longPath = "/assets/";
    for (int i = 0; i < 1000; i++) {
        longPath += "subdir/";
    }
    longPath += "file.txt";
    
    if (longPath.length() > 4096) {
        EXPECT_FALSE(rm.loadFile(longPath).valid());
    }
}
```

### 2. Size Limit Tests

```cpp
TEST(ResourceBoundaryTest, FileSizeLimit) {
    ResourceManager rm;
    rm.setMaxFileSize(10 * 1024 * 1024);  // 10MB
    
    // Create test file larger than limit
    createTestFile("/assets/large.bin", 15 * 1024 * 1024);
    
    auto result = rm.loadFile("large.bin");
    EXPECT_FALSE(result.valid());
    EXPECT_THAT(result.error(), HasSubstr("too large"));
}

TEST(ResourceBoundaryTest, CompressedFileSize) {
    ResourceManager rm;
    rm.setMaxFileSize(10 * 1024 * 1024);
    rm.setDecompressArchives(true);
    
    // Small compressed file that expands beyond limit
    createCompressedFile("/assets/small_but_big.zip", 
                         1024,  // 1KB compressed
                         100 * 1024 * 1024);  // 100MB uncompressed
    
    auto result = rm.loadFile("small_but_big.zip");
    EXPECT_FALSE(result.valid());
}

TEST(ResourceBoundaryTest, StreamingLargeFile) {
    ResourceManager rm;
    rm.setMaxFileSize(10 * 1024 * 1024);
    
    // Test streaming loader with size check during read
    auto stream = rm.streamFile("large.bin");
    
    size_t totalRead = 0;
    char buffer[4096];
    while (auto bytes = stream.read(buffer, sizeof(buffer))) {
        totalRead += bytes;
        if (totalRead > 10 * 1024 * 1024) {
            FAIL() << "Exceeded size limit during streaming";
        }
    }
}

TEST(ResourceBoundaryTest, MultipleFilesTotalSize) {
    ResourceManager rm;
    rm.setMaxTotalMemory(100 * 1024 * 1024);  // 100MB total
    
    // Load multiple files that exceed total when combined
    auto file1 = rm.loadFile("texture1.png");  // 40MB
    auto file2 = rm.loadFile("texture2.png");  // 40MB
    auto file3 = rm.loadFile("texture3.png");  // 40MB
    
    EXPECT_TRUE(file1.valid());
    EXPECT_TRUE(file2.valid());
    EXPECT_FALSE(file3.valid());  // Would exceed limit
}
```

### 3. Symlink Attack Tests

```cpp
TEST(ResourceBoundaryTest, DirectSymlink) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    rm.setFollowSymlinks(false);
    
    // Create symlink: /assets/passwd -> /etc/passwd
    createSymlink("/assets/passwd", "/etc/passwd");
    
    auto result = rm.loadFile("passwd");
    EXPECT_FALSE(result.valid());
    EXPECT_THAT(result.error(), HasSubstr("symlink"));
}

TEST(ResourceBoundaryTest, SymlinkChain) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    rm.setFollowSymlinks(false);
    
    // Create symlink chain: a -> b -> c -> /etc/passwd
    createSymlink("/assets/c", "/etc/passwd");
    createSymlink("/assets/b", "/assets/c");
    createSymlink("/assets/a", "/assets/b");
    
    auto result = rm.loadFile("a");
    EXPECT_FALSE(result.valid());
}

TEST(ResourceBoundaryTest, SymlinkToDirectory) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    rm.setFollowSymlinks(false);
    
    // Create symlink to directory: /assets/etc -> /etc
    createSymlink("/assets/etc", "/etc");
    
    auto result = rm.loadFile("etc/passwd");
    EXPECT_FALSE(result.valid());
}

TEST(ResourceBoundaryTest, SymlinkRace) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    rm.setFollowSymlinks(true);  // Allow symlinks but validate target
    
    // TOCTOU test: symlink changes between check and open
    // This is hard to test reliably but should be documented
    
    // Create valid symlink
    createSymlink("/assets/config", "/assets/config_real");
    createFile("/assets/config_real", "valid config");
    
    // Load should succeed
    auto result = rm.loadFile("config");
    EXPECT_TRUE(result.valid());
}
```

### 4. Type Validation Tests

```cpp
TEST(ResourceBoundaryTest, ExtensionValidation) {
    ResourceManager rm;
    rm.setAllowedExtensions({".png", ".jpg", ".gif"});
    
    EXPECT_TRUE(rm.loadFile("image.png").valid());
    EXPECT_FALSE(rm.loadFile("script.sh").valid());
    EXPECT_FALSE(rm.loadFile("data.txt").valid());
}

TEST(ResourceBoundaryTest, MagicNumberValidation) {
    ResourceManager rm;
    rm.setValidateMagicNumbers(true);
    
    // File with wrong extension
    createFileWithMagic("/assets/fake.png", "PNG", "Actually text content");
    createFileWithMagic("/assets/fake.jpg", "\xFF\xD8\xFF", "Actually text content");
    
    // Should fail magic number check
    EXPECT_FALSE(rm.loadFile("fake.png").valid());
    EXPECT_FALSE(rm.loadFile("fake.jpg").valid());
}

TEST(ResourceBoundaryTest, DoubleExtension) {
    ResourceManager rm;
    rm.setAllowedExtensions({".png", ".jpg"});
    
    // Double extension attack
    EXPECT_FALSE(rm.loadFile("malicious.sh.png").valid());
    EXPECT_FALSE(rm.loadFile("malicious.png.sh").valid());
}

TEST(ResourceBoundaryTest, CaseInsensitiveExtension) {
    ResourceManager rm;
    rm.setAllowedExtensions({".png", ".jpg"});
    rm.setCaseSensitiveExtensions(false);
    
    EXPECT_TRUE(rm.loadFile("image.PNG").valid());
    EXPECT_TRUE(rm.loadFile("image.Png").valid());
    EXPECT_TRUE(rm.loadFile("image.JPG").valid());
}

TEST(ResourceBoundaryTest, HiddenExtension) {
    ResourceManager rm;
    rm.setAllowedExtensions({".png", ".jpg"});
    
    // Hidden extension on Unix (dotfile)
    EXPECT_FALSE(rm.loadFile(".hidden.png").valid());
    
    // Or allow dotfiles but validate
    rm.setAllowDotFiles(true);
    EXPECT_TRUE(rm.loadFile(".hidden.png").valid());
}

TEST(ResourceBoundaryTest, EmptyExtension) {
    ResourceManager rm;
    rm.setAllowedExtensions({".png", ".jpg"});
    
    // File with no extension
    EXPECT_FALSE(rm.loadFile("noextension").valid());
}
```

### 5. Race Condition Tests (TOCTOU)

```cpp
// These tests are inherently flaky but document the attack vector

TEST(ResourceBoundaryTest, TOCTOU_SymlinkSwap) {
    ResourceManager rm;
    rm.setRootDirectory("/assets");
    
    // Create valid file
    createFile("/assets/target", "valid content");
    
    // Start load in background
    auto future = std::async(std::launch::async, [&]() {
        return rm.loadFile("target");
    });
    
    // Try to swap symlink during load (race condition)
    // This is hard to trigger reliably
    usleep(100);  // Tiny delay
    // In real attack: rename target to symlink pointing elsewhere
    
    auto result = future.get();
    // Result should be consistent (either valid file or error)
    // Should not read arbitrary file due to race
}
```

## Mitigation Checklist

- [ ] All paths are canonicalized before use
- [ ] Path traversal sequences are detected and rejected
- [ ] File sizes are checked before AND during read
- [ ] Symlinks are either rejected or their targets are validated
- [ ] File types are validated by content, not just extension
- [ ] Race conditions are mitigated (openat2, O_PATH, etc.)
- [ ] Error messages don't leak path information
- [ ] Resource limits are enforced per-request and globally

## Platform-Specific Notes

### Linux
- Use `openat2()` with `RESOLVE_BENEATH` for path traversal protection
- Use `O_NOFOLLOW` to reject symlinks
- Use `statx()` for efficient metadata checks

### Windows
- Use `CreateFile2()` with `CREATE_ALWAYS`
- Check for `IO_REPARSE_TAG_SYMLINK`
- Be aware of alternate data streams (ADS)

### macOS
- Similar to Linux but use `openat()` with `O_NOFOLLOW`
- Be aware of resource forks

## References

- [CWE-22: Path Traversal](https://cwe.mitre.org/data/definitions/22.html)
- [CWE-73: External Control of File Name](https://cwe.mitre.org/data/definitions/73.html)
- [OWASP Path Traversal](https://owasp.org/www-community/attacks/Path_Traversal)
