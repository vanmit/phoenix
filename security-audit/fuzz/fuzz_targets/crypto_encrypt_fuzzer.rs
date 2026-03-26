//! Crypto Encrypt Fuzzer
//! Tests AES-256-GCM encryption with arbitrary inputs

#![no_main]

use libfuzzer_sys::fuzz_target;
use phoenix_security_core::CryptoModule;

fuzz_target!(|data: &[u8]| {
    // Split input into key and plaintext
    if data.len() < 32 {
        return;
    }
    
    let key = &data[0..32];
    let plaintext = &data[32..];
    
    // Create crypto module
    if let Ok(crypto) = CryptoModule::new(key) {
        // Fuzz encryption
        let _ = crypto.encrypt(plaintext);
        
        // Fuzz base64 encoding
        let _ = crypto.encrypt_to_base64(plaintext);
    }
});
