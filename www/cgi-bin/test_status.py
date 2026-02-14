#!/usr/bin/env python3
import sys

# CGI script that tests status code handling

# Output CGI response with proper CRLF and custom status
sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write("<h1>Status Code Test</h1>")
sys.stdout.write("<p>This tests explicit status code headers</p>")
sys.stdout.write("</body></html>")
