/**
 * ============================================================
 * Webserv Routing Tester
 * Comprehensive test suite for routing functionality
 * ============================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include "../src/http/Router.hpp"
#include "../src/http/HttpRequest.hpp"
#include "../src/config/ServerConfig.hpp"
#include "../src/config/LocationConfig.hpp"

// ============================================================
// Colors
// ============================================================
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[0;34m"
#define NC      "\033[0m"

// ============================================================
// Test Counters
// ============================================================
static int g_pass = 0;
static int g_fail = 0;
static int g_total = 0;

// ============================================================
// Helper Functions
// ============================================================
void printHeader(const std::string& title) {
    std::cout << std::endl;
    std::cout << BLUE << "═══════════════════════════════════════════════════════════" << NC << std::endl;
    std::cout << BLUE << "  " << title << NC << std::endl;
    std::cout << BLUE << "═══════════════════════════════════════════════════════════" << NC << std::endl;
}

void printSubHeader(const std::string& title) {
    std::cout << std::endl;
    std::cout << YELLOW << "──────────────────────────────────────────────────────────" << NC << std::endl;
    std::cout << YELLOW << "  " << title << NC << std::endl;
    std::cout << YELLOW << "──────────────────────────────────────────────────────────" << NC << std::endl;
}

void test_pass(const std::string& name) {
    g_total++;
    g_pass++;
    std::cout << GREEN << "✅ PASS" << NC << " [" << g_total << "] " << name << std::endl;
}

void test_fail(const std::string& name, const std::string& expected, const std::string& got) {
    g_total++;
    g_fail++;
    std::cout << RED << "❌ FAIL" << NC << " [" << g_total << "] " << name << std::endl;
    std::cout << "   " << RED << "Expected: " << expected << NC << std::endl;
    std::cout << "   " << RED << "Got: " << got << NC << std::endl;
}

// Helper to create an HttpRequest from raw request string
HttpRequest createRequest(const std::string& method, const std::string& uri, 
                          const std::string& host = "localhost", size_t contentLength = 0) {
    std::ostringstream oss;
    oss << method << " " << uri << " HTTP/1.1\r\n";
    oss << "Host: " << host << "\r\n";
    if (contentLength > 0) {
        oss << "Content-Length: " << contentLength << "\r\n";
    }
    oss << "\r\n";
    
    HttpRequest req;
    req.parseRequest(oss.str());
    return req;
}

// ============================================================
// Test Setup: Create test servers
// ============================================================
std::vector<ServerConfig> createTestServers() {
    std::vector<ServerConfig> servers;
    
    // Server 1: Main server on port 8080
    ServerConfig server1;
    server1.setPort(8080);
    server1.setServerName("example.com");
    server1.setRoot("/var/www/example");
    server1.setClientMaxBody("10M");
    
    // Location: /
    LocationConfig loc1("/", "/var/www/example");
    loc1.addAllowedMethod("GET");
    loc1.setAutoIndex(false);
    server1.addLocation(loc1);
    
    // Location: /api
    LocationConfig loc2("/api", "/var/www/api");
    loc2.addAllowedMethod("GET");
    loc2.addAllowedMethod("POST");
    server1.addLocation(loc2);
    
    // Location: /api/users
    LocationConfig loc3("/api/users", "/var/www/api/users");
    loc3.addAllowedMethod("GET");
    loc3.addAllowedMethod("POST");
    loc3.addAllowedMethod("DELETE");
    server1.addLocation(loc3);
    
    // Location: /images
    LocationConfig loc4("/images", "/var/www/images");
    loc4.addAllowedMethod("GET");
    loc4.setAutoIndex(true);
    server1.addLocation(loc4);
    
    // Location: /upload
    LocationConfig loc5("/upload", "/var/www/uploads");
    loc5.addAllowedMethod("GET");
    loc5.addAllowedMethod("POST");
    loc5.setUploadEnabled(true);
    loc5.setUploadPath("/var/www/uploads/files");
    loc5.setClientMaxBody("50M");
    server1.addLocation(loc5);
    
    // Location: /cgi-bin
    LocationConfig loc6("/cgi-bin", "/var/www/cgi");
    loc6.addAllowedMethod("GET");
    loc6.addAllowedMethod("POST");
    loc6.setCgiEnabled(true);
    loc6.setCgiExtension(".py");
    loc6.setCgiPath("/usr/bin/python3");
    server1.addLocation(loc6);
    
    // Location: /old-page (redirect)
    LocationConfig loc7("/old-page", "/var/www/example");
    loc7.addAllowedMethod("GET");
    loc7.setRedirect("http://example.com/new-page");
    server1.addLocation(loc7);
    
    servers.push_back(server1);
    
    // Server 2: Secondary server on port 8080 (different host)
    ServerConfig server2;
    server2.setPort(8080);
    server2.setServerName("other.com");
    server2.setRoot("/var/www/other");
    
    LocationConfig loc2_1("/", "/var/www/other");
    loc2_1.addAllowedMethod("GET");
    server2.addLocation(loc2_1);
    
    servers.push_back(server2);
    
    // Server 3: On different port 9090
    ServerConfig server3;
    server3.setPort(9090);
    server3.setServerName("admin.com");
    server3.setRoot("/var/www/admin");
    server3.setClientMaxBody("1M");
    
    LocationConfig loc3_1("/", "/var/www/admin");
    loc3_1.addAllowedMethod("GET");
    loc3_1.addAllowedMethod("POST");
    server3.addLocation(loc3_1);
    
    servers.push_back(server3);
    
    return servers;
}

// ============================================================
// Test 1: Server Selection Tests
// ============================================================
void testServerSelection(Router& router) {
    printSubHeader("Server Selection Tests");
    
    // Test 1.1: Match by exact host
    {
        const ServerConfig* server = router.findServer("example.com", 8080);
        if (server && server->getServerName() == "example.com") {
            test_pass("Match server by exact host header");
        } else {
            test_fail("Match server by exact host header", "example.com", 
                     server ? server->getServerName() : "NULL");
        }
    }
    
    // Test 1.2: Match by host with port
    {
        const ServerConfig* server = router.findServer("example.com:8080", 8080);
        if (server && server->getServerName() == "example.com") {
            test_pass("Match server by host:port format");
        } else {
            test_fail("Match server by host:port format", "example.com",
                     server ? server->getServerName() : "NULL");
        }
    }
    
    // Test 1.3: Match other server same port
    {
        const ServerConfig* server = router.findServer("other.com", 8080);
        if (server && server->getServerName() == "other.com") {
            test_pass("Match other server on same port");
        } else {
            test_fail("Match other server on same port", "other.com",
                     server ? server->getServerName() : "NULL");
        }
    }
    
    // Test 1.4: Default server fallback (unknown host)
    {
        const ServerConfig* server = router.findServer("unknown.com", 8080);
        if (server != NULL) {
            test_pass("Default server fallback for unknown host");
        } else {
            test_fail("Default server fallback for unknown host", "any server", "NULL");
        }
    }
    
    // Test 1.5: Match by port only
    {
        const ServerConfig* server = router.findServer("admin.com", 9090);
        if (server && server->getPort() == 9090) {
            test_pass("Match server on different port (9090)");
        } else {
            test_fail("Match server on different port (9090)", "port 9090",
                     server ? "wrong port" : "NULL");
        }
    }
    
    // Test 1.6: No server for non-existent port
    {
        const ServerConfig* server = router.getDefaultServer(7777);
        if (server == NULL) {
            test_pass("No server for non-configured port");
        } else {
            test_fail("No server for non-configured port", "NULL", "found server");
        }
    }
}

// ============================================================
// Test 2: Location Matching Tests
// ============================================================
void testLocationMatching(Router& router) {
    printSubHeader("Location Matching Tests");
    
    std::vector<ServerConfig> servers = createTestServers();
    const ServerConfig& server = servers[0]; // example.com
    
    // Test 2.1: Root location
    {
        const LocationConfig* loc = router.findLocation("/", server);
        if (loc && loc->getPath() == "/") {
            test_pass("Match root location /");
        } else {
            test_fail("Match root location /", "/", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.2: Exact location path
    {
        const LocationConfig* loc = router.findLocation("/api", server);
        if (loc && loc->getPath() == "/api") {
            test_pass("Match exact location /api");
        } else {
            test_fail("Match exact location /api", "/api", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.3: Nested location - longest prefix match
    {
        const LocationConfig* loc = router.findLocation("/api/users", server);
        if (loc && loc->getPath() == "/api/users") {
            test_pass("Match nested location /api/users (longest prefix)");
        } else {
            test_fail("Match nested location /api/users (longest prefix)", 
                     "/api/users", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.4: Deep path uses longest matching prefix
    {
        const LocationConfig* loc = router.findLocation("/api/users/123/profile", server);
        if (loc && loc->getPath() == "/api/users") {
            test_pass("Deep path /api/users/123/profile matches /api/users");
        } else {
            test_fail("Deep path /api/users/123/profile matches /api/users",
                     "/api/users", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.5: Path that doesn't match any specific location falls to root
    {
        const LocationConfig* loc = router.findLocation("/unknown/path", server);
        if (loc && loc->getPath() == "/") {
            test_pass("Unknown path falls back to root location");
        } else {
            test_fail("Unknown path falls back to root location", 
                     "/", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.6: Path with trailing content after location
    {
        const LocationConfig* loc = router.findLocation("/images/cat.jpg", server);
        if (loc && loc->getPath() == "/images") {
            test_pass("Path /images/cat.jpg matches /images");
        } else {
            test_fail("Path /images/cat.jpg matches /images",
                     "/images", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.7: Query string handling
    {
        const LocationConfig* loc = router.findLocation("/api?query=value", server);
        if (loc && loc->getPath() == "/api") {
            test_pass("Path with query string matches correctly");
        } else {
            test_fail("Path with query string matches correctly",
                     "/api", loc ? loc->getPath() : "NULL");
        }
    }
    
    // Test 2.8: Path boundary check (should NOT match /imagesbackup when location is /images)
    {
        const LocationConfig* loc = router.findLocation("/imagesbackup/test.jpg", server);
        // Should fall back to root "/" not match "/images"
        if (loc && loc->getPath() == "/") {
            test_pass("Path boundary: /imagesbackup does NOT match /images");
        } else {
            test_fail("Path boundary: /imagesbackup does NOT match /images",
                     "/", loc ? loc->getPath() : "NULL");
        }
    }
}

// ============================================================
// Test 3: Method Validation Tests
// ============================================================
void testMethodValidation(Router& router) {
    printSubHeader("Method Validation Tests");
    
    std::vector<ServerConfig> servers = createTestServers();
    const ServerConfig& server = servers[0];
    
    // Get /api location (allows GET, POST)
    const LocationConfig* apiLoc = router.findLocation("/api", server);
    
    // Test 3.1: Allowed method GET
    {
        if (router.isMethodAllowed("GET", *apiLoc)) {
            test_pass("GET method allowed on /api");
        } else {
            test_fail("GET method allowed on /api", "true", "false");
        }
    }
    
    // Test 3.2: Allowed method POST
    {
        if (router.isMethodAllowed("POST", *apiLoc)) {
            test_pass("POST method allowed on /api");
        } else {
            test_fail("POST method allowed on /api", "true", "false");
        }
    }
    
    // Test 3.3: Disallowed method DELETE on /api
    {
        if (!router.isMethodAllowed("DELETE", *apiLoc)) {
            test_pass("DELETE method NOT allowed on /api");
        } else {
            test_fail("DELETE method NOT allowed on /api", "false", "true");
        }
    }
    
    // Get /api/users location (allows GET, POST, DELETE)
    const LocationConfig* usersLoc = router.findLocation("/api/users", server);
    
    // Test 3.4: DELETE allowed on /api/users
    {
        if (router.isMethodAllowed("DELETE", *usersLoc)) {
            test_pass("DELETE method allowed on /api/users");
        } else {
            test_fail("DELETE method allowed on /api/users", "true", "false");
        }
    }
    
    // Get root location (only GET)
    const LocationConfig* rootLoc = router.findLocation("/", server);
    
    // Test 3.5: POST not allowed on root
    {
        if (!router.isMethodAllowed("POST", *rootLoc)) {
            test_pass("POST method NOT allowed on root /");
        } else {
            test_fail("POST method NOT allowed on root /", "false", "true");
        }
    }
}

// ============================================================
// Test 4: Path Resolution Tests
// ============================================================
void testPathResolution(Router& router) {
    printSubHeader("Path Resolution Tests");
    
    std::vector<ServerConfig> servers = createTestServers();
    const ServerConfig& server = servers[0];
    
    // Test 4.1: Root path resolution
    {
        const LocationConfig* loc = router.findLocation("/", server);
        std::string resolved = router.resolvePath("/", *loc);
        if (resolved == "/var/www/example/") {
            test_pass("Root path / resolves to /var/www/example/");
        } else {
            test_fail("Root path / resolves correctly", "/var/www/example/", resolved);
        }
    }
    
    // Test 4.2: Root path with file
    {
        const LocationConfig* loc = router.findLocation("/index.html", server);
        std::string resolved = router.resolvePath("/index.html", *loc);
        if (resolved == "/var/www/example/index.html") {
            test_pass("Path /index.html resolves correctly");
        } else {
            test_fail("Path /index.html resolves correctly", 
                     "/var/www/example/index.html", resolved);
        }
    }
    
    // Test 4.3: Nested location path resolution
    {
        const LocationConfig* loc = router.findLocation("/api/users", server);
        std::string resolved = router.resolvePath("/api/users", *loc);
        // /api/users should use its own root and strip the location prefix
        if (resolved.find("/var/www/api/users") != std::string::npos) {
            test_pass("Path /api/users resolves with correct root");
        } else {
            test_fail("Path /api/users resolves with correct root",
                     "/var/www/api/users...", resolved);
        }
    }
    
    // Test 4.4: Path with file after location
    {
        const LocationConfig* loc = router.findLocation("/images/cat.jpg", server);
        std::string resolved = router.resolvePath("/images/cat.jpg", *loc);
        if (resolved == "/var/www/images/cat.jpg") {
            test_pass("Path /images/cat.jpg resolves correctly");
        } else {
            test_fail("Path /images/cat.jpg resolves correctly",
                     "/var/www/images/cat.jpg", resolved);
        }
    }
    
    // Test 4.5: Deep nested path
    {
        const LocationConfig* loc = router.findLocation("/api/users/123", server);
        std::string resolved = router.resolvePath("/api/users/123", *loc);
        if (resolved == "/var/www/api/users/123") {
            test_pass("Deep path /api/users/123 resolves correctly");
        } else {
            test_fail("Deep path /api/users/123 resolves correctly",
                     "/var/www/api/users/123", resolved);
        }
    }
}

// ============================================================
// Test 5: Body Size Validation Tests
// ============================================================
void testBodySizeValidation(Router& router) {
    printSubHeader("Body Size Validation Tests");
    
    std::vector<ServerConfig> servers = createTestServers();
    const ServerConfig& server = servers[0];
    
    // Test 5.1: Parse size with K suffix
    {
        size_t size = router.parseBodySize("10K");
        if (size == 10 * 1024) {
            test_pass("Parse 10K = 10240 bytes");
        } else {
            std::ostringstream oss;
            oss << size;
            test_fail("Parse 10K = 10240 bytes", "10240", oss.str());
        }
    }
    
    // Test 5.2: Parse size with M suffix
    {
        size_t size = router.parseBodySize("5M");
        if (size == 5 * 1024 * 1024) {
            test_pass("Parse 5M = 5242880 bytes");
        } else {
            std::ostringstream oss;
            oss << size;
            test_fail("Parse 5M = 5242880 bytes", "5242880", oss.str());
        }
    }
    
    // Test 5.3: Parse size with G suffix
    {
        size_t size = router.parseBodySize("1G");
        if (size == 1UL * 1024 * 1024 * 1024) {
            test_pass("Parse 1G = 1073741824 bytes");
        } else {
            std::ostringstream oss;
            oss << size;
            test_fail("Parse 1G = 1073741824 bytes", "1073741824", oss.str());
        }
    }
    
    // Test 5.4: Parse size without suffix (bytes)
    {
        size_t size = router.parseBodySize("1000");
        if (size == 1000) {
            test_pass("Parse 1000 = 1000 bytes");
        } else {
            std::ostringstream oss;
            oss << size;
            test_fail("Parse 1000 = 1000 bytes", "1000", oss.str());
        }
    }
    
    // Test 5.5: Body size check - within limit
    {
        const LocationConfig* loc = router.findLocation("/upload", server);
        // Upload location has 50M limit
        if (router.checkBodySize(1000000, *loc)) { // 1MB
            test_pass("1MB body within 50M limit");
        } else {
            test_fail("1MB body within 50M limit", "true", "false");
        }
    }
    
    // Test 5.6: Body size check - at limit
    {
        const LocationConfig* loc = router.findLocation("/upload", server);
        if (router.checkBodySize(50 * 1024 * 1024, *loc)) { // exactly 50MB
            test_pass("50MB body at 50M limit");
        } else {
            test_fail("50MB body at 50M limit", "true", "false");
        }
    }
    
    // Test 5.7: Body size check - exceeds limit
    {
        const LocationConfig* loc = router.findLocation("/upload", server);
        if (!router.checkBodySize(51 * 1024 * 1024, *loc)) { // 51MB > 50MB
            test_pass("51MB body exceeds 50M limit");
        } else {
            test_fail("51MB body exceeds 50M limit", "false", "true");
        }
    }
}

// ============================================================
// Test 6: CGI and Upload Detection Tests
// ============================================================
void testCgiUploadDetection(Router& router) {
    printSubHeader("CGI and Upload Detection Tests");
    
    std::vector<ServerConfig> servers = createTestServers();
    const ServerConfig& server = servers[0];
    
    // Test 6.1: CGI request detection - .py extension
    {
        const LocationConfig* loc = router.findLocation("/cgi-bin", server);
        if (router.isCgiRequest("/cgi-bin/script.py", *loc)) {
            test_pass("Detect CGI request for .py file");
        } else {
            test_fail("Detect CGI request for .py file", "true", "false");
        }
    }
    
    // Test 6.2: Non-CGI request
    {
        const LocationConfig* loc = router.findLocation("/cgi-bin", server);
        if (!router.isCgiRequest("/cgi-bin/readme.txt", *loc)) {
            test_pass("Non-.py file is not CGI request");
        } else {
            test_fail("Non-.py file is not CGI request", "false", "true");
        }
    }
    
    // Test 6.3: CGI disabled location
    {
        const LocationConfig* loc = router.findLocation("/api", server);
        if (!router.isCgiRequest("/api/script.py", *loc)) {
            test_pass("CGI disabled - .py file not treated as CGI");
        } else {
            test_fail("CGI disabled - .py file not treated as CGI", "false", "true");
        }
    }
    
    // Test 6.4: Upload request detection - POST to upload location
    {
        const LocationConfig* loc = router.findLocation("/upload", server);
        if (router.isUploadRequest("POST", *loc)) {
            test_pass("Detect upload request (POST to /upload)");
        } else {
            test_fail("Detect upload request (POST to /upload)", "true", "false");
        }
    }
    
    // Test 6.5: Non-upload request - GET to upload location
    {
        const LocationConfig* loc = router.findLocation("/upload", server);
        if (!router.isUploadRequest("GET", *loc)) {
            test_pass("GET to /upload is not upload request");
        } else {
            test_fail("GET to /upload is not upload request", "false", "true");
        }
    }
    
    // Test 6.6: POST to non-upload location
    {
        const LocationConfig* loc = router.findLocation("/api", server);
        if (!router.isUploadRequest("POST", *loc)) {
            test_pass("POST to /api is not upload request");
        } else {
            test_fail("POST to /api is not upload request", "false", "true");
        }
    }
}

// ============================================================
// Test 7: Redirect Tests
// ============================================================
void testRedirects(Router& router) {
    printSubHeader("Redirect Tests");
    
    std::vector<ServerConfig> servers = createTestServers();
    
    // Test 7.1: Redirect detection
    {
        HttpRequest req = createRequest("GET", "/old-page", "example.com");
        RouteResult result = router.route(req, 8080);
        
        if (result.isRedirect && result.statusCode == 301) {
            test_pass("Redirect detected with 301 status");
        } else {
            std::ostringstream oss;
            oss << "isRedirect=" << result.isRedirect << ", status=" << result.statusCode;
            test_fail("Redirect detected with 301 status", 
                     "isRedirect=true, status=301", oss.str());
        }
    }
    
    // Test 7.2: Redirect URL
    {
        HttpRequest req = createRequest("GET", "/old-page", "example.com");
        RouteResult result = router.route(req, 8080);
        
        if (result.redirectUrl == "http://example.com/new-page") {
            test_pass("Redirect URL is correct");
        } else {
            test_fail("Redirect URL is correct",
                     "http://example.com/new-page", result.redirectUrl);
        }
    }
}

// ============================================================
// Test 8: Full Routing Flow Tests
// ============================================================
void testFullRoutingFlow(Router& router) {
    printSubHeader("Full Routing Flow Tests");
    
    // Test 8.1: Complete successful routing
    {
        HttpRequest req = createRequest("GET", "/images/photo.jpg", "example.com");
        RouteResult result = router.route(req, 8080);
        
        if (result.found && result.statusCode == 200 && result.location != NULL) {
            test_pass("Complete routing success for GET /images/photo.jpg");
        } else {
            std::ostringstream oss;
            oss << "found=" << result.found << ", status=" << result.statusCode;
            test_fail("Complete routing success for GET /images/photo.jpg",
                     "found=true, status=200", oss.str());
        }
    }
    
    // Test 8.2: Method not allowed - 405
    {
        HttpRequest req = createRequest("DELETE", "/images/photo.jpg", "example.com");
        RouteResult result = router.route(req, 8080);
        
        if (result.statusCode == 405) {
            test_pass("405 Method Not Allowed for DELETE on /images");
        } else {
            std::ostringstream oss;
            oss << result.statusCode;
            test_fail("405 Method Not Allowed for DELETE on /images", "405", oss.str());
        }
    }
    
    // Test 8.3: No server for port - 500
    {
        HttpRequest req = createRequest("GET", "/", "localhost");
        RouteResult result = router.route(req, 7777);
        
        if (!result.found && result.statusCode == 500) {
            test_pass("500 error for non-configured port");
        } else {
            std::ostringstream oss;
            oss << "found=" << result.found << ", status=" << result.statusCode;
            test_fail("500 error for non-configured port",
                     "found=false, status=500", oss.str());
        }
    }
    
    // Test 8.4: Body too large - 413
    {
        HttpRequest req = createRequest("POST", "/upload/file.txt", "example.com", 
                                        60 * 1024 * 1024); // 60MB
        RouteResult result = router.route(req, 8080);
        
        if (result.statusCode == 413) {
            test_pass("413 Request Entity Too Large for 60MB upload");
        } else {
            std::ostringstream oss;
            oss << result.statusCode;
            test_fail("413 Request Entity Too Large for 60MB upload", "413", oss.str());
        }
    }
    
    // Test 8.5: Route result contains correct server reference
    {
        HttpRequest req = createRequest("GET", "/", "other.com");
        RouteResult result = router.route(req, 8080);
        
        if (result.server && result.server->getServerName() == "other.com") {
            test_pass("Route result contains correct server (other.com)");
        } else {
            test_fail("Route result contains correct server (other.com)",
                     "other.com", result.server ? result.server->getServerName() : "NULL");
        }
    }
    
    // Test 8.6: Route result contains matched path
    {
        HttpRequest req = createRequest("GET", "/api/users/123", "example.com");
        RouteResult result = router.route(req, 8080);
        
        if (result.matchedPath == "/api/users") {
            test_pass("Route result contains matched path /api/users");
        } else {
            test_fail("Route result contains matched path /api/users",
                     "/api/users", result.matchedPath);
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    printHeader("Webserv Routing Tester - Comprehensive Test Suite");
    
    // Create test servers and initialize router
    std::vector<ServerConfig> servers = createTestServers();
    Router router(servers);
    
    // Run all test suites
    testServerSelection(router);
    testLocationMatching(router);
    testMethodValidation(router);
    testPathResolution(router);
    testBodySizeValidation(router);
    testCgiUploadDetection(router);
    testRedirects(router);
    testFullRoutingFlow(router);
    
    // Print summary
    printHeader("SUMMARY");
    std::cout << std::endl;
    std::cout << "  Total:  " << g_total << std::endl;
    std::cout << "  Passed: " << GREEN << g_pass << NC << std::endl;
    std::cout << "  Failed: " << RED << g_fail << NC << std::endl;
    
    std::cout << std::endl;
    
    if (g_fail == 0) {
        std::cout << GREEN << "  ✅ ALL TESTS PASSED! Routing is ready." << NC << std::endl;
        std::cout << std::endl;
        std::cout << "  📋 NEXT STEPS:" << std::endl;
        std::cout << "     1. Integrate Router with ServerManager" << std::endl;
        std::cout << "     2. Connect routing to request handlers" << std::endl;
        std::cout << "     3. Test with real HTTP requests" << std::endl;
    } else {
        std::cout << RED << "  ❌ SOME TESTS FAILED! Fix issues before proceeding." << NC << std::endl;
        std::cout << std::endl;
        std::cout << "  📋 ISSUES TO FIX:" << std::endl;
        std::cout << "     Review failed tests above and fix Router logic" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << BLUE << "═══════════════════════════════════════════════════════════" << NC << std::endl;
    
    return (g_fail > 0) ? 1 : 0;
}
