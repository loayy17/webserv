#!/usr/bin/env python3
import sys

# CGI script that generates a large response

# Output CGI response with proper CRLF
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write("<h1>Large Response Test</h1>")
# Generate ~100KB of data
for i in range(1000):
    sys.stdout.write(f"<p>Line {i}: " + "A" * 100 + "</p>\n")
sys.stdout.write("</body></html>")
