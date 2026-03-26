//! Audit Logger Fuzzer
//! Tests audit logging with various inputs

#![no_main]

use libfuzzer_sys::fuzz_target;
use phoenix_security_core::AuditLogger;

fuzz_target!(|data: &[u8]| {
    if data.len() < 32 {
        return;
    }
    
    let hmac_key = &data[0..32];
    let log_data = &data[32..];
    
    // Create logger
    let logger = AuditLogger::new(hmac_key);
    
    // Split log_data into event_type and details
    if let Some(null_pos) = log_data.iter().position(|&b| b == 0) {
        if let Ok(event_type) = std::str::from_utf8(&log_data[0..null_pos]) {
            if let Ok(details) = std::str::from_utf8(&log_data[null_pos + 1..]) {
                // Log event
                let entry = logger.log(event_type, details);
                
                // Verify entry
                let _ = logger.verify_entry(&entry);
                
                // Get all entries
                let _ = logger.get_entries();
                
                // Verify all
                let _ = logger.verify_all();
            }
        }
    }
    
    // Test with empty strings
    let _ = logger.log("", "");
    let _ = logger.log("TEST", "");
    let _ = logger.log("", "details");
    
    // Test with very long strings
    let long_string = "A".repeat(10000);
    let _ = logger.log(&long_string, &long_string);
    
    // Test with special characters
    let _ = logger.log("EVENT\0WITH\0NULLS", "DETAILS");
    let _ = logger.log("EVENT\nWITH\nNEWLINES", "DETAILS\r\nWITH\r\nCRLF");
    let _ = logger.log("EVENT|WITH|PIPES", "DETAILS|WITH|PIPES");
});
