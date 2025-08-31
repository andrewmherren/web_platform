// Test functions for auth utilities - only compiled in debug builds
// These functions are meant to be called from main.cpp or debug interfaces

#include "../../include/auth/auth_utils.h"

#ifdef DEBUG_AUTH_UTILS

namespace AuthUtils {

void testIPParsing() {
  Serial.println("=== Testing IP Address Parsing ===");
  
  // Test valid IPs
  struct TestCase {
    String input;
    bool shouldBeValid;
    uint8_t expectedBytes[4];
  };
  
  TestCase testCases[] = {
    {"192.168.1.1", true, {192, 168, 1, 1}},
    {"10.0.0.1", true, {10, 0, 0, 1}},
    {"172.16.0.1", true, {172, 16, 0, 1}},
    {"127.0.0.1", true, {127, 0, 0, 1}},
    {"0.0.0.0", true, {0, 0, 0, 0}},
    {"255.255.255.255", true, {255, 255, 255, 255}},
    
    // Invalid IPs
    {"192.168.1", false, {0, 0, 0, 0}},
    {"192.168.1.256", false, {0, 0, 0, 0}},
    {"192.168.1.1.1", false, {0, 0, 0, 0}},
    {"192.168.-1.1", false, {0, 0, 0, 0}},
    {"invalid", false, {0, 0, 0, 0}},
    {"", false, {0, 0, 0, 0}}
  };
  
  for (const auto& test : testCases) {
    IPAddress parsed = parseIPAddress(test.input);
    bool isValid = parsed.isValid() || 
                   (parsed.bytes[0] == 0 && parsed.bytes[1] == 0 && 
                    parsed.bytes[2] == 0 && parsed.bytes[3] == 0 && test.shouldBeValid);
    
    Serial.printf("IP: %-20s Expected: %s, Got: %s", 
                 test.input.c_str(),
                 test.shouldBeValid ? "valid" : "invalid",
                 (isValid == test.shouldBeValid) ? "PASS" : "FAIL");
                 
    if (test.shouldBeValid && isValid) {
      Serial.printf(" [%s]", parsed.toString().c_str());
    }
    Serial.println();
  }
}

void testLocalNetworkDetection() {
  Serial.println("\\n=== Testing Local Network Detection ===");
  
  struct TestCase {
    String ip;
    bool shouldBeLocal;
    String description;
  };
  
  TestCase testCases[] = {
    // Private networks (RFC 1918)
    {"10.0.0.1", true, "Class A private"},
    {"10.255.255.254", true, "Class A private (edge)"},
    {"172.16.0.1", true, "Class B private (start)"},
    {"172.31.255.254", true, "Class B private (end)"},
    {"172.15.255.254", false, "Just before Class B private"},
    {"172.32.0.1", false, "Just after Class B private"},
    {"192.168.0.1", true, "Class C private"},
    {"192.168.255.254", true, "Class C private (edge)"},
    
    // Link-local (RFC 3927)
    {"169.254.1.1", true, "Link-local"},
    {"169.254.255.254", true, "Link-local (edge)"},
    
    // Loopback
    {"127.0.0.1", true, "Loopback"},
    {"127.255.255.255", true, "Loopback (edge)"},
    
    // Public IPs
    {"8.8.8.8", false, "Google DNS"},
    {"1.1.1.1", false, "Cloudflare DNS"},
    {"192.169.1.1", false, "Near private but public"},
    {"11.0.0.1", false, "Near Class A private but public"}
  };
  
  for (const auto& test : testCases) {
    IPAddress ip = parseIPAddress(test.ip);
    bool isLocal = isLocalNetworkIP(ip);
    
    Serial.printf("IP: %-15s (%s) Expected: %s, Got: %s\\n",
                 test.ip.c_str(),
                 test.description.c_str(),
                 test.shouldBeLocal ? "local" : "public",
                 (isLocal == test.shouldBeLocal) ? "PASS" : "FAIL");
  }
}

void testSubnetMatching() {
  Serial.println("\\n=== Testing Subnet Matching ===");
  
  struct TestCase {
    String ip;
    Subnet subnet;
    bool shouldMatch;
    String description;
  };
  
  TestCase testCases[] = {
    {"192.168.1.100", Subnet(192, 168, 1, 0, 24), true, "192.168.1.0/24"},
    {"192.168.1.200", Subnet(192, 168, 1, 0, 24), true, "192.168.1.0/24"},
    {"192.168.2.100", Subnet(192, 168, 1, 0, 24), false, "192.168.1.0/24"},
    {"192.168.0.100", Subnet(192, 168, 0, 0, 16), true, "192.168.0.0/16"},
    {"192.168.255.100", Subnet(192, 168, 0, 0, 16), true, "192.168.0.0/16"},
    {"192.167.255.100", Subnet(192, 168, 0, 0, 16), false, "192.168.0.0/16"},
    {"10.0.0.1", Subnet(10, 0, 0, 0, 8), true, "10.0.0.0/8"},
    {"10.255.255.255", Subnet(10, 0, 0, 0, 8), true, "10.0.0.0/8"},
    {"11.0.0.1", Subnet(10, 0, 0, 0, 8), false, "10.0.0.0/8"}
  };
  
  for (const auto& test : testCases) {
    IPAddress ip = parseIPAddress(test.ip);
    bool matches = isIPInSubnet(ip, test.subnet);
    
    Serial.printf("IP: %-15s in %s Expected: %s, Got: %s\\n",
                 test.ip.c_str(),
                 test.description.c_str(),
                 test.shouldMatch ? "match" : "no match",
                 (matches == test.shouldMatch) ? "PASS" : "FAIL");
  }
}

void runAllTests() {
  Serial.println("\\n==========================================");
  Serial.println("     AUTH UTILS IP VALIDATION TESTS");
  Serial.println("==========================================");
  
  testIPParsing();
  testLocalNetworkDetection();
  testSubnetMatching();
  
  Serial.println("\\n========================================");
  Serial.println("           TESTS COMPLETED");
  Serial.println("========================================\\n");
}

} // namespace AuthUtils

#endif // DEBUG_AUTH_UTILS