#!/usr/bin/env python3
import sys
import os

# CGI script that reads and displays cookies

# Get cookie header
cookies = os.environ.get("HTTP_COOKIE", "")

# Output CGI response with proper CRLF
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write(f"<h1>Cookie Test</h1>")
sys.stdout.write(f"<p>Cookies: {cookies}</p>")
sys.stdout.write("</body></html>")
