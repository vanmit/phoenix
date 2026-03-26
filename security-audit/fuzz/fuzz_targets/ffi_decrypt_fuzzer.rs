//! FFI Decrypt Fuzzer
//! Tests the C FFI interface for crypto decrypt with malformed inputs
//! This specifically targets CWE-119 (buffer overflow) and CWE-190 (integer overflow)

#![no_main]

use libfuzzer_sys::fuzz_target;
use phoenix_security_core::ffi::*;
use std::ptr;

fuzz_target!(|data: &[u8]| {
    // Need at least key + encrypted data header
    if data.len() < 32 + 4 {
        return;
    }
    
    let key = &data[0..32];
    let encrypted_data = &data[32..];
    
    // Create crypto module via FFI
    let crypto_handle = unsafe {
        crypto_module_new(key.as_ptr(), key.len())
    };
    
    if crypto_handle.is_null() {
        return;
    }
    
    // Test various malformed inputs
    
    // Test 1: Empty encrypted data
    let mut out_len: usize = 0;
    unsafe {
        let _ = crypto_decrypt(crypto_handle, ptr::null(), 0, &mut out_len);
    }
    
    // Test 2: Very small encrypted data
    if encrypted_data.len() >= 1 {
        unsafe {
            let _ = crypto_decrypt(crypto_handle, encrypted_data.as_ptr(), 1, &mut out_len);
        }
    }
    
    // Test 3: Malformed nonce length (potential integer overflow)
    if encrypted_data.len() >= 4 {
        // Extract potentially malicious nonce_len
        let nonce_len = u32::from_le_bytes([
            encrypted_data[0],
            encrypted_data[1],
            encrypted_data[2],
            encrypted_data[3],
        ]) as usize;
        
        // Test with various sizes
        for &test_len in &[0, 1, 4, 8, 16, encrypted_data.len()] {
            unsafe {
                let _ = crypto_decrypt(crypto_handle, encrypted_data.as_ptr(), test_len, &mut out_len);
            }
        }
        
        // Test with huge nonce_len (integer overflow test)
        let mut huge_data = vec![0xFFu8; 8];
        huge_data[0..4].copy_from_slice(&u32::MAX.to_le_bytes());
        unsafe {
            let _ = crypto_decrypt(crypto_handle, huge_data.as_ptr(), huge_data.len(), &mut out_len);
        }
    }
    
    // Test 4: Full length test
    if !encrypted_data.is_empty() {
        unsafe {
            let _ = crypto_decrypt(crypto_handle, encrypted_data.as_ptr(), encrypted_data.len(), &mut out_len);
        }
    }
    
    // Cleanup
    unsafe {
        crypto_module_destroy(crypto_handle);
    }
});
