#!/usr/bin/env python3
import sys
import os

# CGI script that handles POST requests

# Read request body
content_length = os.environ.get("CONTENT_LENGTH", "0")
body = ""
if content_length and content_length != "0":
    body = sys.stdin.read(int(content_length))

# Get content type
content_type = os.environ.get("CONTENT_TYPE", "")

# Output CGI response with proper CRLF
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write(f"<h1>POST Handler</h1>")
sys.stdout.write(f"<p>Content-Type: {content_type}</p>")
sys.stdout.write(f"<p>Content-Length: {content_length}</p>")
sys.stdout.write(f"<p>Body received: {body}</p>")
sys.stdout.write("</body></html>")
