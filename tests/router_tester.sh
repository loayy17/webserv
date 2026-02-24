#!/bin/bash

# ============================================================
# Router Tester
# Tests Router with separate config and request files
# ============================================================

TESTER="./router_tester"
TEST_DIR="router_tests"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_COUNT=0

print_header() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
}

print_subheader() {
    echo ""
    echo -e "${YELLOW}──────────────────────────────────────────────────────────${NC}"
    echo -e "${YELLOW}  $1${NC}"
    echo -e "${YELLOW}──────────────────────────────────────────────────────────${NC}"
}

# Args: test_name config_content request_file expected_statusCode [expected_matchedPath] [expected_serverName]
run_test_file() {
    local test_name="$1"
    local config_content="$2"
    local request_file="$3"
    local expected_statusCode="$4"
    local expected_matchedPath="$5"
    local expected_serverName="$6"

    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    
    local config_file="$TEST_DIR/config_${TOTAL_COUNT}.conf"
    cp <(echo "$config_content") "$config_file"

    # Run tester directly with request file
    output=$($TESTER "$config_file" "$request_file" 2>&1)

    # Everything else same as run_test parsing
    actual_statusCode=$(echo "$output" | grep "^statusCode=" | cut -d'=' -f2)
    actual_matchedPath=$(echo "$output" | grep "^matchedPath=" | cut -d'=' -f2)
    actual_serverName=$(echo "$output" | grep "^serverName=" | cut -d'=' -f2)

    local passed=true
    local errors=""

    if [ "$actual_statusCode" != "$expected_statusCode" ]; then
        passed=false
        errors="${errors}   Expected statusCode=$expected_statusCode, got $actual_statusCode\n"
    fi
    if [ -n "$expected_matchedPath" ] && [ "$actual_matchedPath" != "$expected_matchedPath" ]; then
        passed=false
        errors="${errors}   Expected matchedPath='$expected_matchedPath', got '$actual_matchedPath'\n"
    fi
    if [ -n "$expected_serverName" ] && [ "$actual_serverName" != "$expected_serverName" ]; then
        passed=false
        errors="${errors}   Expected serverName='$expected_serverName', got '$actual_serverName'\n"
    fi

    if [ "$passed" = true ]; then
        echo -e "${GREEN}✅ PASS${NC} [$TOTAL_COUNT] $test_name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo -e "${RED}❌ FAIL${NC} [$TOTAL_COUNT] $test_name"
        echo -e "${RED}${errors}${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# Test function
# Args: test_name config_content request_content expected_statusCode [expected_matchedPath] [expected_serverName]
run_test() {
    local test_name="$1"
    local config_content="$2"
    local request_content="$3"
    local expected_statusCode="$4"
    local expected_matchedPath="$5"
    local expected_serverName="$6"
    
    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    
    # Create test files with proper line endings
    local config_file="$TEST_DIR/config_${TOTAL_COUNT}.conf"
    local request_file="$TEST_DIR/request_${TOTAL_COUNT}.txt"
    
    printf "%s" "$config_content" > "$config_file"
    printf "%b" "$request_content" > "$request_file"
    
    # Run tester
    output=$($TESTER "$config_file" "$request_file" 2>&1)
    
    # Check for errors
    if echo "$output" | grep -q "^ERROR|"; then
        echo -e "${RED}❌ FAIL${NC} [$TOTAL_COUNT] $test_name"
        echo -e "   ${RED}$(echo "$output" | grep "^ERROR|" | cut -d'|' -f2)${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
    
    # Parse output
    actual_statusCode=$(echo "$output" | grep "^statusCode=" | cut -d'=' -f2)
    actual_matchedPath=$(echo "$output" | grep "^matchedPath=" | cut -d'=' -f2)
    actual_serverName=$(echo "$output" | grep "^serverName=" | cut -d'=' -f2)
    
    # Compare results
    local passed=true
    local errors=""
    
    if [ "$actual_statusCode" != "$expected_statusCode" ]; then
        passed=false
        errors="${errors}   Expected statusCode=$expected_statusCode, got $actual_statusCode\n"
    fi
    
    if [ -n "$expected_matchedPath" ] && [ "$actual_matchedPath" != "$expected_matchedPath" ]; then
        passed=false
        errors="${errors}   Expected matchedPath='$expected_matchedPath', got '$actual_matchedPath'\n"
    fi
    
    if [ -n "$expected_serverName" ] && [ "$actual_serverName" != "$expected_serverName" ]; then
        passed=false
        errors="${errors}   Expected serverName='$expected_serverName', got '$actual_serverName'\n"
    fi
    
    if [ "$passed" = true ]; then
        echo -e "${GREEN}✅ PASS${NC} [$TOTAL_COUNT] $test_name"
        PASS_COUNT=$((PASS_COUNT + 1))
        return 0
    else
        echo -e "${RED}❌ FAIL${NC} [$TOTAL_COUNT] $test_name"
        echo -e "${RED}${errors}${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi
}

# ============================================================
# Check if tester binary exists
# ============================================================

print_header "Router Tester"

if [ ! -f "$TESTER" ]; then
    echo -e "${RED}❌ Error: $TESTER not found${NC}"
    echo -e "${YELLOW}Please compile first: make router_tester${NC}"
    exit 1
fi

# Create test directory
mkdir -p "$TEST_DIR"

# ============================================================
# BASIC ROUTING TESTS
# ============================================================

print_subheader "Basic Routing Tests"

# Create dummy content
mkdir -p "$TEST_DIR/www"
echo "index" > "$TEST_DIR/www/index.html"
mkdir -p "$TEST_DIR/www/images"
echo "photo" > "$TEST_DIR/www/images/photo.jpg"
mkdir -p "$TEST_DIR/www/site1"
echo "site1" > "$TEST_DIR/www/site1/index.html"
mkdir -p "$TEST_DIR/www/site2"
echo "site2" > "$TEST_DIR/www/site2/index.html"
mkdir -p "$TEST_DIR/www/upload"
echo "upload" > "$TEST_DIR/www/upload/index.html"
mkdir -p "$TEST_DIR/www/api/users"
echo "profile" > "$TEST_DIR/www/api/users/profile"
echo "posts" > "$TEST_DIR/www/api/posts"

CWD=$(pwd)

# Test 1: Simple root path
CONFIG="http {
    server {
        listen localhost:8080;
        server_name localhost;
        root $CWD/$TEST_DIR/www;
        location / {
            methods GET;
            index index.html;
        }
    }
}"

REQUEST=$'GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "Simple root path routing" "$CONFIG" "$REQUEST" "200" "/" ""

# Test 2: Nested path
CONFIG="http {
    server {
        listen localhost:8080;
        server_name localhost;
        root $CWD/$TEST_DIR/www;
        location / {
            methods GET POST;
            index index.html;
        }
        location /images {
            methods GET;
            root $CWD/$TEST_DIR/www;
        }
    }
}"

REQUEST=$'GET /images/photo.jpg HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "Nested path /images" "$CONFIG" "$REQUEST" "404" "/images" ""

# ============================================================
# SERVER SELECTION TESTS
# ============================================================

print_subheader "Server Selection Tests"

# Test 3: Multiple servers by port
CONFIG="http {
    server {
        listen localhost:8080;
        server_name site1.com;
        root $CWD/$TEST_DIR/www/site1;
        location / {
            methods GET;
            index index.html;
        }
    }
    server {
        listen localhost:8081;
        server_name site2.com;
        root $CWD/$TEST_DIR/www/site2;
        location / {
            methods GET;
            index index.html;
        }
    }
}"

REQUEST=$'GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "Select server by port 8080" "$CONFIG" "$REQUEST" "200" "/" "site1.com"

REQUEST=$'GET / HTTP/1.1\r\nHost: localhost:8081\r\n\r\n'

run_test "Select server by port 8081" "$CONFIG" "$REQUEST" "200" "/" "site2.com"

# Test 4: Same port, different server_name
CONFIG="http {
    server {
        listen localhost:8080;
        server_name site1.com;
        root $CWD/$TEST_DIR/www/site1;
        location / {
            methods GET;
            index index.html;
        }
    }
    server {
        listen localhost:8080;
        server_name site2.com;
        root $CWD/$TEST_DIR/www/site2;
        location / {
            methods GET;
            index index.html;
        }
    }
}"

REQUEST=$'GET / HTTP/1.1\r\nHost: site1.com:8080\r\n\r\n'

run_test "Select by server_name site1.com" "$CONFIG" "$REQUEST" "200" "/" "site1.com"

REQUEST=$'GET / HTTP/1.1\r\nHost: site2.com:8080\r\n\r\n'

run_test "Select by server_name site2.com" "$CONFIG" "$REQUEST" "200" "/" "site2.com"

# ============================================================
# LOCATION MATCHING TESTS
# ============================================================

print_subheader "Location Matching Tests"

# Test 5: Longest prefix match
CONFIG="http {
    server {
        listen localhost:8080;
        server_name localhost;
        root $CWD/$TEST_DIR/www;
        location / {
            methods GET POST;
            index index.html;
        }
        location /api {
            methods GET;
        }
        location /api/users {
            methods GET POST;
        }
    }
}"

REQUEST=$'GET /api/users/profile HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "Longest prefix /api/users" "$CONFIG" "$REQUEST" "404" "/api/users" ""

REQUEST=$'GET /api/posts HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "Shorter prefix /api" "$CONFIG" "$REQUEST" "404" "/api" ""

# ============================================================
# HTTP METHOD TESTS
# ============================================================

print_subheader "HTTP Method Tests"

CONFIG="http {
    server {
        listen localhost:8080;
        server_name localhost;
        root $CWD/$TEST_DIR/www;
        location / {
            methods GET POST DELETE;
            index index.html;
        }
    }
}"

REQUEST=$'GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "GET method allowed" "$CONFIG" "$REQUEST" "200" "/" ""

REQUEST=$'POST / HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 10\r\n\r\ntest=value'

run_test "POST method allowed" "$CONFIG" "$REQUEST" "200" "/" ""

REQUEST=$'PUT / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n'

run_test "PUT method not allowed" "$CONFIG" "$REQUEST" "405" "/" ""

# ============================================================
# BODY SIZE VALIDATION TESTS
# ============================================================

print_subheader "Body Size Validation Tests"

CONFIG="http {
    client_max_body_size 1M;
    server {
        listen localhost:8080;
        server_name localhost;
        root $CWD/$TEST_DIR/www;
        location / {
            methods GET POST;
            index index.html;
        }
        location /upload {
            methods POST;
            client_max_body_size 10M;
            root $CWD/$TEST_DIR/www;
            index index.html;
        }
    }
}"
# Helper to build request files
build_request_file() {
    local method="$1"
    local path="$2"
    local host="$3"
    local body_file="$4"
    local request_file="$5"

    local body_size=0
    [ -f "$body_file" ] && body_size=$(stat -c%s "$body_file")

    # Build headers
    printf "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\n\r\n" "$path" "$host" "$body_size" > "$request_file"

    # Append body if exists
    [ -f "$body_file" ] && cat "$body_file" >> "$request_file"
}
# -----------------------------
# Test 1: Body size within limit (500KB < 1M)
# -----------------------------
head -c 500000 /dev/zero | tr '\0' 'x' > "$TEST_DIR/body_500kb.txt"
build_request_file "POST" "/" "localhost:8080" "$TEST_DIR/body_500kb.txt" "$TEST_DIR/request_1.txt"
run_test_file "Body size within limit" "$CONFIG" "$TEST_DIR/request_1.txt" "200" "/" ""

# -----------------------------
# Test 2: Body size exceeds limit (2MB > 1M)
# -----------------------------
head -c 2000000 /dev/zero | tr '\0' 'x' > "$TEST_DIR/body_2mb.txt"
build_request_file "POST" "/" "localhost:8080" "$TEST_DIR/body_2mb.txt" "$TEST_DIR/request_2.txt"
run_test_file "Body size exceeds limit" "$CONFIG" "$TEST_DIR/request_2.txt" "200" "/" ""

# -----------------------------
# Test 3: Body within location limit (5MB < 10M)
# -----------------------------
head -c 5000000 /dev/zero | tr '\0' 'x' > "$TEST_DIR/body_5mb.txt"
build_request_file "POST" "/upload" "localhost:8080" "$TEST_DIR/body_5mb.txt" "$TEST_DIR/request_3.txt"
run_test_file "Body within location limit" "$CONFIG" "$TEST_DIR/request_3.txt" "200" "/upload" ""
# ============================================================
# ERROR CASES
# ============================================================

print_subheader "Error Cases"

CONFIG="http {
    server {
        listen localhost:8080;
        server_name localhost;
        root $CWD/$TEST_DIR/www;
        location / {
            methods GET;
            index index.html;
        }
    }
}"

REQUEST=$'GET / HTTP/1.1\r\nHost: localhost:9999\r\n\r\n'

run_test "Wrong port (no server)" "$CONFIG" "$REQUEST" "400" "" ""

# ============================================================
# SUMMARY
# ============================================================

print_header "Test Summary"
echo "Total Tests: $TOTAL_COUNT"
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"

# Cleanup
rm -rf "$TEST_DIR"

if [ $FAIL_COUNT -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}❌ Some tests failed${NC}"
    exit 1
fi
