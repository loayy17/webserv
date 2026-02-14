#!/usr/bin/env python3
import sys
import os

# CGI script that echoes request information

# Read request body (if any)
content_length = os.environ.get("CONTENT_LENGTH", "0")
body = ""
if content_length and content_length != "0":
    body = sys.stdin.read(int(content_length))

# Get environment variables
method = os.environ.get("REQUEST_METHOD", "")
query_string = os.environ.get("QUERY_STRING", "")
path_info = os.environ.get("PATH_INFO", "")

# Output CGI response with proper CRLF
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<html><body>")
sys.stdout.write(f"<h1>Echo Test</h1>")
sys.stdout.write(f"<p>Method: {method}</p>")
sys.stdout.write(f"<p>Query String: {query_string}</p>")
sys.stdout.write(f"<p>Path Info: {path_info}</p>")
sys.stdout.write(f"<p>Body: {body}</p>")
sys.stdout.write("</body></html>")
