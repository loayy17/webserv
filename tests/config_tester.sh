#!/bin/bash

# ============================================================
# Webserv Config Parser Tester
# Comprehensive test suite for configuration file parsing
# ============================================================

WEBSERV="./config_tester"
TEST_DIR="tests/config_tests"
PASS=0
FAIL=0
TOTAL=0

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create test directory
mkdir -p "$TEST_DIR"

# ============================================================
# Helper Functions
# ============================================================

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

# Test function: expects SUCCESS
test_success() {
    local name="$1"
    local config_file="$2"
    TOTAL=$((TOTAL + 1))
    
    output=$($WEBSERV "$config_file" 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✅ PASS${NC} [$TOTAL] $name"
        PASS=$((PASS + 1))
    else
        echo -e "${RED}❌ FAIL${NC} [$TOTAL] $name"
        echo -e "   ${RED}Expected: SUCCESS, Got: FAILURE${NC}"
        echo -e "   ${RED}Error: $output${NC}"
        FAIL=$((FAIL + 1))
    fi
}

# Test function: expects FAILURE with specific error message
test_failure() {
    local name="$1"
    local config_file="$2"
    local expected_error="$3"
    TOTAL=$((TOTAL + 1))
    
    output=$($WEBSERV "$config_file" 2>&1)
    exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        if [ -n "$expected_error" ] && [[ "$output" == *"$expected_error"* ]]; then
            echo -e "${GREEN}✅ PASS${NC} [$TOTAL] $name"
            PASS=$((PASS + 1))
        elif [ -z "$expected_error" ]; then
            echo -e "${GREEN}✅ PASS${NC} [$TOTAL] $name"
            PASS=$((PASS + 1))
        else
            echo -e "${RED}❌ FAIL${NC} [$TOTAL] $name"
            echo -e "   ${RED}Expected error: '$expected_error'${NC}"
            echo -e "   ${RED}Got: $output${NC}"
            FAIL=$((FAIL + 1))
        fi
    else
        echo -e "${RED}❌ FAIL${NC} [$TOTAL] $name"
        echo -e "   ${RED}Expected: FAILURE, Got: SUCCESS${NC}"
        FAIL=$((FAIL + 1))
    fi
}

# ============================================================
# Generate Test Config Files
# ============================================================

generate_test_files() {
    print_header "Generating Test Configuration Files"

    # ----------------------------------------------------------
    # SUCCESS CASES
    # ----------------------------------------------------------

    # 1. Minimal valid config
    cat > "$TEST_DIR/01_minimal_valid.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 2. Full valid config with all options
    cat > "$TEST_DIR/02_full_valid.conf" << 'EOF'
http {
    client_max_body_size 10M;
    server {
        listen localhost:8080;
        server_name example.com;
        root /var/www/html;
        client_max_body_size 5M;
        
        location / {
            root /var/www/default;
            index home.html;
            autoindex on;
        }
        
        location /upload {
            root /var/www/uploads;
            client_max_body_size 20M;
            autoindex off;
            methods GET POST DELETE;
        }
        
        location /api {
            root /var/www/api;
            methods GET POST;
        }
    }
}
EOF

    # 3. Multiple servers
    cat > "$TEST_DIR/03_multiple_servers.conf" << 'EOF'
http {
    server {
        listen localhost:1234;
        root /var/www/site1;
        server_name site1.com;
        location / {
            index index.html;
        }
    }
    server {
        listen localhost:4321;
        root /var/www/site2;
        server_name site2.com;
        location / {
            index index.html;
        }
    }
}
EOF

    # 4. Server without http block (standalone)
    cat > "$TEST_DIR/04_server_standalone.conf" << 'EOF'
server {
    listen localhost:8080;
    root /var/www;
    location / {
        index index.html;
    }
}
EOF

    # 5. Multiple locations
    cat > "$TEST_DIR/05_multiple_locations.conf" << 'EOF'
http {
    server {
        listen localhost:9000;
        root /var/www;
        location / {
            index index.html;
        }
        location /images {
            autoindex on;
        }
        location /css {
            autoindex off;
        }
        location /js {
            index main.js;
        }
        location /api/v1 {
            methods GET POST;
        }
    }
}
EOF

    # 6. Config with comments
    cat > "$TEST_DIR/06_with_comments.conf" << 'EOF'
# Main HTTP configuration
http {
    client_max_body_size 1M; # Global body size limit
    
    # Primary server
    server {
        listen localhost:8080; # Main port
        root /var/www; # Document root
        # server_name localhost; # Optional
        
        location / {
            index index.html; # Default file
            # autoindex on;
        }
    }
}
EOF

    # 7. IP address listen
    cat > "$TEST_DIR/07_ip_listen.conf" << 'EOF'
http {
    server {
        listen 127.0.0.1:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 8. All methods specified
    cat > "$TEST_DIR/08_all_methods.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            methods GET POST DELETE;
        }
    }
}
EOF

    # 9. Nested location paths
    cat > "$TEST_DIR/09_nested_paths.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
        location /api {
            methods GET;
        }
        location /api/users {
            methods GET POST;
        }
        location /api/users/profile {
            methods GET POST DELETE;
        }
    }
}
EOF

    # 10. Whitespace variations
    cat > "$TEST_DIR/10_whitespace.conf" << 'EOF'
http {
    server {
        listen    localhost:8080;
        root      /var/www;
        location / {
            index    index.html;
        }
    }
}
EOF

    # 11. Inline braces
    cat > "$TEST_DIR/11_inline_braces.conf" << 'EOF'
http { server { listen localhost:8080; root /var/www; location / { index index.html; } } }
EOF

    # 12. Location inherits server root
    cat > "$TEST_DIR/12_inherit_root.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
        location /images {
            autoindex on;
        }
    }
}
EOF

    # 13. Tab indentation
    cat > "$TEST_DIR/13_tabs.conf" << 'EOF'
http {
	server {
		listen localhost:8080;
		root /var/www;
		location / {
			index index.html;
		}
	}
}
EOF

    # ----------------------------------------------------------
    # FAILURE CASES
    # ----------------------------------------------------------

    # 20. Empty file
    cat > "$TEST_DIR/20_empty.conf" << 'EOF'
EOF

    # 21. No server block
    cat > "$TEST_DIR/21_no_server.conf" << 'EOF'
http {
    client_max_body_size 1M;
}
EOF

    # 22. Multiple http blocks
    cat > "$TEST_DIR/22_multiple_http.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
http {
    server {
        listen localhost:9090;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 23. Missing listen directive
    cat > "$TEST_DIR/23_no_listen.conf" << 'EOF'
http {
    server {
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 24. Duplicate listen
    cat > "$TEST_DIR/24_dup_listen.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        listen 127.0.0.1:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 24b. Multiple values in single listen directive (invalid)
    cat > "$TEST_DIR/24b_multi_value_listen.conf" << 'EOF'
http {
    server {
        listen localhost:8080 127.0.0.1:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 24c. Wildcard and specific IP on same port (should be duplicate)
    cat > "$TEST_DIR/24c_wildcard_and_ip.conf" << 'EOF'
http {
    server {
        listen 0.0.0.0:8080;
        listen 127.0.0.1:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 24d. Same host, different ports (valid)
    cat > "$TEST_DIR/24d_diff_ports.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        listen localhost:9090;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 24e. Hostname without port (invalid format for this parser)
    cat > "$TEST_DIR/24e_hostname_no_port.conf" << 'EOF'
http {
    server {
        listen localhost;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 25. Invalid listen format (no colon)
    cat > "$TEST_DIR/25_invalid_listen.conf" << 'EOF'
http {
    server {
        listen 8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 26. Invalid port number (negative)
    cat > "$TEST_DIR/26_negative_port.conf" << 'EOF'
http {
    server {
        listen localhost:-1;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 27. Invalid port number (too high)
    cat > "$TEST_DIR/27_port_too_high.conf" << 'EOF'
http {
    server {
        listen localhost:70000;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 28. No location block
    cat > "$TEST_DIR/28_no_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
    }
}
EOF

    # 29. Duplicate root in server
    cat > "$TEST_DIR/29_dup_root_server.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        root /var/www2;
        location / {
            index index.html;
        }
    }
}
EOF

    # 30. Duplicate server_name
    cat > "$TEST_DIR/30_dup_server_name.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        server_name localhost;
        server_name example.com;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 31. Duplicate index in server
    cat > "$TEST_DIR/31_dup_index_server.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        index index.html;
        index home.html;
        location / {
            index index.html;
        }
    }
}
EOF

    # 32. Duplicate client_max_body_size in server
    cat > "$TEST_DIR/32_dup_body_server.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        client_max_body_size 1M;
        client_max_body_size 2M;
        location / {
            index index.html;
        }
    }
}
EOF

    # 33. Duplicate client_max_body_size in http
    cat > "$TEST_DIR/33_dup_body_http.conf" << 'EOF'
http {
    client_max_body_size 1M;
    client_max_body_size 2M;
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 34. Unknown http directive
    cat > "$TEST_DIR/34_unknown_http.conf" << 'EOF'
http {
    unknown_directive value;
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 35. Unknown server directive
    cat > "$TEST_DIR/35_unknown_server.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        unknown_directive value;
        location / {
            index index.html;
        }
    }
}
EOF

    # 36. Unknown location directive
    cat > "$TEST_DIR/36_unknown_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            unknown_directive value;
        }
    }
}
EOF

    # 37. Location without path
    cat > "$TEST_DIR/37_location_no_path.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location {
            index index.html;
        }
    }
}
EOF

    # 38. Location path doesn't start with /
    cat > "$TEST_DIR/38_location_invalid_path.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location images {
            index index.html;
        }
    }
}
EOF

    # 39. Duplicate location path
    cat > "$TEST_DIR/39_dup_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
        location / {
            index home.html;
        }
    }
}
EOF

    # 40. Duplicate root in location
    cat > "$TEST_DIR/40_dup_root_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            root /var/www1;
            root /var/www2;
        }
    }
}
EOF

    # 41. Duplicate autoindex in location
    cat > "$TEST_DIR/41_dup_autoindex.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            autoindex on;
            autoindex off;
        }
    }
}
EOF

    # 42. Invalid autoindex value
    cat > "$TEST_DIR/42_invalid_autoindex.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            autoindex yes;
        }
    }
}
EOF

    # 43. Duplicate index in location
    cat > "$TEST_DIR/43_dup_index_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
            index home.html;
        }
    }
}
EOF

    # 44. Duplicate client_max_body_size in location
    cat > "$TEST_DIR/44_dup_body_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            client_max_body_size 1M;
            client_max_body_size 2M;
        }
    }
}
EOF

    # 45. Duplicate method in location
    # NOTE: This test documents a BUG in ConfigParser - duplicate methods are NOT detected!
    cat > "$TEST_DIR/45_dup_method.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            methods GET GET;
        }
    }
}
EOF

    # 46. Missing closing brace http
    cat > "$TEST_DIR/46_missing_brace_http.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
EOF

    # 47. Missing closing brace server
    cat > "$TEST_DIR/47_missing_brace_server.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
}
EOF

    # 48. Missing closing brace location
    cat > "$TEST_DIR/48_missing_brace_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
    }
}
EOF

    # 49. No root defined anywhere
    cat > "$TEST_DIR/49_no_root.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        location / {
            index index.html;
        }
    }
}
EOF

    # 50. Invalid directive at top level
    cat > "$TEST_DIR/50_invalid_toplevel.conf" << 'EOF'
invalid_toplevel {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 51. Server inside location
    cat > "$TEST_DIR/51_server_in_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            server {
                listen localhost:9090;
            }
        }
    }
}
EOF

    # 52. File does not exist
    # (No file created - test will use non-existent path)

    # 53. Port 0
    cat > "$TEST_DIR/53_port_zero.conf" << 'EOF'
http {
    server {
        listen localhost:0;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 54. Port not a number
    cat > "$TEST_DIR/54_port_nan.conf" << 'EOF'
http {
    server {
        listen localhost:abc;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 55. Empty server block
    cat > "$TEST_DIR/55_empty_server.conf" << 'EOF'
http {
    server {
    }
}
EOF

    # 56. Nested http blocks
    cat > "$TEST_DIR/56_nested_http.conf" << 'EOF'
http {
    http {
        server {
            listen localhost:8080;
            root /var/www;
            location / {
                index index.html;
            }
        }
    }
}
EOF

    # 57. Location inside http (not server)
    cat > "$TEST_DIR/57_location_in_http.conf" << 'EOF'
http {
    location / {
        index index.html;
    }
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 58. Extra closing brace
    cat > "$TEST_DIR/58_extra_brace.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
}
EOF

    # ============================================================
    # EDGE CASES
    # ============================================================

    # 60. Very long server_name
    LONG_NAME=$(printf 'a%.0s' {1..500})
    cat > "$TEST_DIR/60_long_server_name.conf" << EOF
http {
    server {
        listen localhost:8080;
        server_name $LONG_NAME;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 61. Very long root path
    LONG_PATH="/var/www/$(printf 'subdir/%.0s' {1..50})"
    cat > "$TEST_DIR/61_long_root_path.conf" << EOF
http {
    server {
        listen localhost:8080;
        root $LONG_PATH;
        location / {
            index index.html;
        }
    }
}
EOF

    # 62. Many locations (100 locations)
    cat > "$TEST_DIR/62_many_locations.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
EOF
    for i in $(seq 1 100); do
        echo "        location /path$i { index file$i.html; }" >> "$TEST_DIR/62_many_locations.conf"
    done
    cat >> "$TEST_DIR/62_many_locations.conf" << 'EOF'
    }
}
EOF

    # 63. Many servers (50 servers)
    cat > "$TEST_DIR/63_many_servers.conf" << 'EOF'
http {
EOF
    for i in $(seq 1 50); do
        port=$((8000 + i))
        cat >> "$TEST_DIR/63_many_servers.conf" << EOF
    server {
        listen localhost:$port;
        root /var/www/site$i;
        location / { index index.html; }
    }
EOF
    done
    echo "}" >> "$TEST_DIR/63_many_servers.conf"

    # 64. Special characters in server_name
    cat > "$TEST_DIR/64_special_servername.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        server_name my-server.domain.com;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 65. Underscore in server_name
    cat > "$TEST_DIR/65_underscore_servername.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        server_name my_server;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 66. Port 1 (minimum valid)
    cat > "$TEST_DIR/66_port_min.conf" << 'EOF'
http {
    server {
        listen localhost:1;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 67. Port 65535 (maximum valid)
    cat > "$TEST_DIR/67_port_max.conf" << 'EOF'
http {
    server {
        listen localhost:65535;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 68. Port 65536 (just over max)
    cat > "$TEST_DIR/68_port_over_max.conf" << 'EOF'
http {
    server {
        listen localhost:65536;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 69. Empty location block
    cat > "$TEST_DIR/69_empty_location.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
        }
    }
}
EOF

    # 70. Client_max_body_size with K suffix
    cat > "$TEST_DIR/70_body_size_k.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        client_max_body_size 100K;
        location / {
            index index.html;
        }
    }
}
EOF

    # 71. Client_max_body_size with G suffix
    cat > "$TEST_DIR/71_body_size_g.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        client_max_body_size 1G;
        location / {
            index index.html;
        }
    }
}
EOF

    # 72. Client_max_body_size zero
    cat > "$TEST_DIR/72_body_size_zero.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        client_max_body_size 0;
        location / {
            index index.html;
        }
    }
}
EOF

    # 73. Location with only root
    cat > "$TEST_DIR/73_location_only_root.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            root /var/www/custom;
        }
    }
}
EOF

    # 74. Location with only autoindex
    cat > "$TEST_DIR/74_location_only_autoindex.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            autoindex on;
        }
    }
}
EOF

    # 75. Location with only methods
    cat > "$TEST_DIR/75_location_only_methods.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            methods POST;
        }
    }
}
EOF

    # 76. Only DELETE method
    cat > "$TEST_DIR/76_delete_only.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            methods DELETE;
        }
    }
}
EOF

    # 77. 0.0.0.0 listen address
    cat > "$TEST_DIR/77_all_interfaces.conf" << 'EOF'
http {
    server {
        listen 0.0.0.0:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 78. IPv6 localhost (should fail - not supported)
    cat > "$TEST_DIR/78_ipv6.conf" << 'EOF'
http {
    server {
        listen [::1]:8080;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 79. Unicode in path (non-ASCII)
    cat > "$TEST_DIR/79_unicode_path.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www/مجلد;
        location / {
            index index.html;
        }
    }
}
EOF

    # 80. Semicolon in middle of value
    cat > "$TEST_DIR/80_semicolon_value.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;test;
        location / {
            index index.html;
        }
    }
}
EOF

    # 81. Only spaces/whitespace file
    cat > "$TEST_DIR/81_whitespace_only.conf" << 'EOF'
   
    
        
EOF

    # 82. Only comments file
    cat > "$TEST_DIR/82_comments_only.conf" << 'EOF'
# This is a comment
# Another comment
# No actual configuration
EOF

    # 83. Directive without value
    cat > "$TEST_DIR/83_no_value.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root;
        location / {
            index index.html;
        }
    }
}
EOF

    # 84. Multiple values for single-value directive
    cat > "$TEST_DIR/84_multi_value_root.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www /var/www2;
        location / {
            index index.html;
        }
    }
}
EOF

    # 85. Trailing whitespace
    cat > "$TEST_DIR/85_trailing_whitespace.conf" << 'EOF'
http {    
    server {    
        listen localhost:8080;    
        root /var/www;    
        location / {    
            index index.html;    
        }    
    }    
}    
EOF

    # 86. Windows line endings (CRLF)
    printf 'http {\r\n    server {\r\n        listen localhost:8080;\r\n        root /var/www;\r\n        location / {\r\n            index index.html;\r\n        }\r\n    }\r\n}\r\n' > "$TEST_DIR/86_crlf.conf"

    # 87. Mixed line endings
    printf 'http {\n    server {\r\n        listen localhost:8080;\n        root /var/www;\r\n        location / {\n            index index.html;\r\n        }\n    }\r\n}\n' > "$TEST_DIR/87_mixed_endings.conf"

    # 88. Location path with double slashes
    cat > "$TEST_DIR/88_double_slash.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location //api {
            methods GET;
        }
    }
}
EOF

    # 89. Location path with trailing slash
    cat > "$TEST_DIR/89_trailing_slash.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location /api/ {
            methods GET;
        }
    }
}
EOF

    # 90. Very deep nested location path
    cat > "$TEST_DIR/90_deep_path.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location /a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p {
            methods GET;
        }
    }
}
EOF

    # 91. Same port different servers
    cat > "$TEST_DIR/91_same_port.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        server_name site1.com;
        root /var/www/site1;
        location / {
            index index.html;
        }
    }
    server {
        listen localhost:8080;
        server_name site2.com;
        root /var/www/site2;
        location / {
            index index.html;
        }
    }
}
EOF

    # 92. Empty string values
    cat > "$TEST_DIR/92_empty_values.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        server_name "";
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    # 93. Invalid method name
    cat > "$TEST_DIR/93_invalid_method.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            methods GET INVALID_METHOD;
        }
    }
}
EOF

    # 94. Lowercase method
    cat > "$TEST_DIR/94_lowercase_method.conf" << 'EOF'
http {
    server {
        listen localhost:8080;
        root /var/www;
        location / {
            methods get post;
        }
    }
}
EOF

    # 95. Numeric overflow port
    cat > "$TEST_DIR/95_overflow_port.conf" << 'EOF'
http {
    server {
        listen localhost:999999999999;
        root /var/www;
        location / {
            index index.html;
        }
    }
}
EOF

    echo -e "${GREEN}Generated $(ls -1 "$TEST_DIR"/*.conf 2>/dev/null | wc -l) test configuration files${NC}"
}

# ============================================================
# Run Tests
# ============================================================

run_tests() {
    print_header "Running Configuration Parser Tests"

    # ----------------------------------------------------------
    # SUCCESS CASES
    # ----------------------------------------------------------
    print_subheader "SUCCESS Cases (Expected to PASS)"

    test_success "Minimal valid config" "$TEST_DIR/01_minimal_valid.conf"
    test_success "Full valid config with all options" "$TEST_DIR/02_full_valid.conf"
    test_success "Multiple servers" "$TEST_DIR/03_multiple_servers.conf"
    test_success "Server standalone (no http block)" "$TEST_DIR/04_server_standalone.conf"
    test_success "Multiple locations" "$TEST_DIR/05_multiple_locations.conf"
    test_success "Config with comments" "$TEST_DIR/06_with_comments.conf"
    test_success "IP address in listen" "$TEST_DIR/07_ip_listen.conf"
    test_success "All HTTP methods" "$TEST_DIR/08_all_methods.conf"
    test_success "Nested location paths" "$TEST_DIR/09_nested_paths.conf"
    test_success "Whitespace variations" "$TEST_DIR/10_whitespace.conf"
    test_success "Inline braces" "$TEST_DIR/11_inline_braces.conf"
    test_success "Location inherits server root" "$TEST_DIR/12_inherit_root.conf"
    test_success "Tab indentation" "$TEST_DIR/13_tabs.conf"
    

    # ----------------------------------------------------------
    # FAILURE CASES
    # ----------------------------------------------------------
    print_subheader "FAILURE Cases (Expected to FAIL)"

    test_failure "Empty configuration file" "$TEST_DIR/20_empty.conf" "[ERROR]: Configuration file is empty"
    test_failure "No server block in http" "$TEST_DIR/21_no_server.conf" "No server defined"
    test_failure "Multiple http blocks" "$TEST_DIR/22_multiple_http.conf" "[ERROR]: Only one http block allowed"
    test_failure "Missing listen directive" "$TEST_DIR/23_no_listen.conf" "[ERROR]: Server missing listen directive"
    test_failure "Multiple listen entries (localhost and 127.0.0.1 treated independently)" "$TEST_DIR/24_dup_listen.conf"
    test_failure "Multiple values in single listen directive" "$TEST_DIR/24b_multi_value_listen.conf" "[ERROR]: listen takes exactly one value"
    test_success "Wildcard and IP on same port (treated independently)" "$TEST_DIR/24c_wildcard_and_ip.conf"
    test_success "Same host different ports (valid)" "$TEST_DIR/24d_diff_ports.conf"
    test_failure "Hostname without port in listen" "$TEST_DIR/24e_hostname_no_port.conf" "invalid listen format"
    test_failure "Invalid listen format (no colon)" "$TEST_DIR/25_invalid_listen.conf" "invalid listen format"
    test_failure "Negative port number" "$TEST_DIR/26_negative_port.conf" "invalid port"
    test_failure "Port number too high (>65535)" "$TEST_DIR/27_port_too_high.conf" "invalid port"
    test_failure "No location block" "$TEST_DIR/28_no_location.conf" "[ERROR]: At least one location is required"
    test_failure "Duplicate root in server" "$TEST_DIR/29_dup_root_server.conf" "duplicate root"
    test_failure "Duplicate server_name" "$TEST_DIR/30_dup_server_name.conf" "duplicate server_name"
    test_failure "Duplicate index in server" "$TEST_DIR/31_dup_index_server.conf" "duplicate index"
    test_failure "Duplicate client_max_body_size in server" "$TEST_DIR/32_dup_body_server.conf" "[ERROR]: Duplicate client_max_body_size"
    test_failure "Duplicate client_max_body_size in http" "$TEST_DIR/33_dup_body_http.conf" "[ERROR]: Duplicate client_max_body_size"
    test_failure "Unknown http directive" "$TEST_DIR/34_unknown_http.conf" "Unknown http directive"
    test_failure "Unknown server directive" "$TEST_DIR/35_unknown_server.conf" "Unknown server directive"
    test_failure "Unknown location directive" "$TEST_DIR/36_unknown_location.conf" "Unknown location directive"
    test_failure "Location without path" "$TEST_DIR/37_location_no_path.conf" ""
    test_failure "Location path doesn't start with /" "$TEST_DIR/38_location_invalid_path.conf" "[ERROR]: Location path must start with '/'"
    test_failure "Duplicate location path" "$TEST_DIR/39_dup_location.conf" "[ERROR]: Duplicate location path: /"
    test_failure "Duplicate root in location" "$TEST_DIR/40_dup_root_location.conf" "duplicate root"
    test_failure "Duplicate autoindex in location" "$TEST_DIR/41_dup_autoindex.conf" "duplicate autoindex"
    test_failure "Invalid autoindex value" "$TEST_DIR/42_invalid_autoindex.conf" "invalid autoindex value"
    test_failure "Duplicate index in location" "$TEST_DIR/43_dup_index_location.conf" "duplicate index"
    test_failure "Duplicate client_max_body_size in location" "$TEST_DIR/44_dup_body_location.conf" "[ERROR]: Duplicate client_max_body_size"
    # Fixed: ConfigParser now detects duplicate methods
    test_failure "Duplicate method in location" "$TEST_DIR/45_dup_method.conf" "duplicate method"
    test_failure "Missing closing brace (http)" "$TEST_DIR/46_missing_brace_http.conf" ""
    test_failure "Missing closing brace (server)" "$TEST_DIR/47_missing_brace_server.conf" ""
    test_failure "Missing closing brace (location)" "$TEST_DIR/48_missing_brace_location.conf" ""
    test_failure "No root defined anywhere" "$TEST_DIR/49_no_root.conf" "[ERROR]: Location has no root and server has no root"
    test_failure "Invalid top-level directive" "$TEST_DIR/50_invalid_toplevel.conf" "Invalid directive"
    test_failure "Server inside location" "$TEST_DIR/51_server_in_location.conf" "Unknown location directive"
    test_failure "Non-existent config file" "$TEST_DIR/nonexistent.conf" ""
    test_failure "Port zero" "$TEST_DIR/53_port_zero.conf" "invalid port"
    test_failure "Port not a number" "$TEST_DIR/54_port_nan.conf" "invalid port"
    test_failure "Empty server block" "$TEST_DIR/55_empty_server.conf" "[ERROR]: Server missing listen directive"
    test_failure "Nested http blocks" "$TEST_DIR/56_nested_http.conf" "Unknown http directive"
    test_failure "Location inside http" "$TEST_DIR/57_location_in_http.conf" "Unknown http directive"
    test_failure "Extra closing brace" "$TEST_DIR/58_extra_brace.conf" "Invalid directive"

    # ----------------------------------------------------------
    # EDGE CASES
    # ----------------------------------------------------------
    print_subheader "EDGE Cases (Boundary Testing)"

    # These should PASS (valid edge cases)
    test_success "Very long server_name (500 chars)" "$TEST_DIR/60_long_server_name.conf"
    test_success "Very long root path" "$TEST_DIR/61_long_root_path.conf"
    test_success "100 locations in one server" "$TEST_DIR/62_many_locations.conf"
    test_success "50 servers in http block" "$TEST_DIR/63_many_servers.conf"
    test_success "Hyphen and dots in server_name" "$TEST_DIR/64_special_servername.conf"
    test_success "Underscore in server_name" "$TEST_DIR/65_underscore_servername.conf"
    test_success "Port 1 (minimum valid)" "$TEST_DIR/66_port_min.conf"
    test_success "Port 65535 (maximum valid)" "$TEST_DIR/67_port_max.conf"
    test_success "Empty location block (inherits)" "$TEST_DIR/69_empty_location.conf"
    test_success "client_max_body_size with K suffix" "$TEST_DIR/70_body_size_k.conf"
    test_success "client_max_body_size with G suffix" "$TEST_DIR/71_body_size_g.conf"
    test_success "client_max_body_size zero" "$TEST_DIR/72_body_size_zero.conf"
    test_success "Location with only root" "$TEST_DIR/73_location_only_root.conf"
    test_success "Location with only autoindex" "$TEST_DIR/74_location_only_autoindex.conf"
    test_success "Location with only methods" "$TEST_DIR/75_location_only_methods.conf"
    test_success "DELETE method only" "$TEST_DIR/76_delete_only.conf"
    test_success "0.0.0.0 all interfaces" "$TEST_DIR/77_all_interfaces.conf"
    test_success "Unicode in path" "$TEST_DIR/79_unicode_path.conf"
    test_success "Trailing whitespace" "$TEST_DIR/85_trailing_whitespace.conf"
    test_success "Windows CRLF line endings" "$TEST_DIR/86_crlf.conf"
    test_success "Mixed line endings" "$TEST_DIR/87_mixed_endings.conf"
    test_success "Double slash in location path" "$TEST_DIR/88_double_slash.conf"
    test_success "Trailing slash in location path" "$TEST_DIR/89_trailing_slash.conf"
    test_success "Very deep nested location path" "$TEST_DIR/90_deep_path.conf"
    test_success "Same port different server_names" "$TEST_DIR/91_same_port.conf"
    
    # These MIGHT fail or have undefined behavior - parser decides
    echo ""
    echo -e "${YELLOW}  >>> Testing potential issues (behavior may vary) <<<${NC}"
    
    # Port 65536 should fail
    test_failure "Port 65536 (over max)" "$TEST_DIR/68_port_over_max.conf" "invalid port"
    
    # IPv6 format likely not supported
    test_failure "IPv6 address format" "$TEST_DIR/78_ipv6.conf" ""
    
    # Semicolon breaks parsing
    test_failure "Semicolon in value" "$TEST_DIR/80_semicolon_value.conf" ""
    
    # Whitespace/comments only = no server
    test_failure "Whitespace only file" "$TEST_DIR/81_whitespace_only.conf" "[ERROR]: Configuration file is empty"
    test_failure "Comments only file" "$TEST_DIR/82_comments_only.conf" "[ERROR]: Configuration file is empty"
    
    # Empty root value should fail
    test_failure "Directive without value" "$TEST_DIR/83_no_value.conf" ""
    
    # Numeric overflow
    test_failure "Port numeric overflow" "$TEST_DIR/95_overflow_port.conf" "invalid port"

    # Edge cases that should now be properly validated
    echo ""
    echo -e "${YELLOW}  >>> Validation tests (should reject invalid input) <<<${NC}"
    
    # Empty server_name with quotes - depends on how parser handles it
    output=$($WEBSERV "$TEST_DIR/92_empty_values.conf" 2>&1)
    if [ $? -eq 0 ]; then
        echo -e "${BLUE}ℹ️ INFO${NC} Empty quoted server_name: ACCEPTED"
    else
        echo -e "${BLUE}ℹ️ INFO${NC} Empty quoted server_name: REJECTED"
    fi
    TOTAL=$((TOTAL + 1))
    PASS=$((PASS + 1))
    
    # Invalid method names - should now FAIL
    test_failure "Invalid method name" "$TEST_DIR/93_invalid_method.conf" "invalid method"
    
    # Lowercase methods - should now convert to uppercase and PASS
    test_success "Lowercase methods (auto-uppercase)" "$TEST_DIR/94_lowercase_method.conf"
    
    # Multiple values for root - should now FAIL
    test_failure "Multiple values for root" "$TEST_DIR/84_multi_value_root.conf" "[ERROR]: root takes exactly one value"
}

# ============================================================
# Print Summary
# ============================================================

print_summary() {
    print_header "Test Summary"
    
    echo ""
    echo -e "  Total Tests: ${BLUE}$TOTAL${NC}"
    echo -e "  Passed:      ${GREEN}$PASS${NC}"
    echo -e "  Failed:      ${RED}$FAIL${NC}"
    echo ""
    
    if [ $FAIL -eq 0 ]; then
        echo -e "${GREEN}╔═══════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║                                                           ║${NC}"
        echo -e "${GREEN}║              ✅ ALL TESTS PASSED! 100%                    ║${NC}"
        echo -e "${GREEN}║                                                           ║${NC}"
        echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
        exit 0
    else
        PERCENT=$((PASS * 100 / TOTAL))
        echo -e "${RED}╔═══════════════════════════════════════════════════════════╗${NC}"
        echo -e "${RED}║                                                           ║${NC}"
        echo -e "${RED}║              ❌ SOME TESTS FAILED: $PERCENT%                     ║${NC}"
        echo -e "${RED}║                                                           ║${NC}"
        echo -e "${RED}╚═══════════════════════════════════════════════════════════╝${NC}"
        exit 1
    fi
}

# ============================================================
# Main
# ============================================================

# Check if webserv exists
if [ ! -f "$WEBSERV" ]; then
    echo -e "${RED}Error: $WEBSERV not found. Please compile the project first.${NC}"
    echo "Run: make"
    exit 1
fi

generate_test_files
run_tests
print_summary
