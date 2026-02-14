#!/usr/bin/env python3
"""
Test CGI Script - Slow Response (for timeout testing)
WARNING: This script intentionally waits to test server timeout handling
"""
import os
import time

# Get wait time from query (default 5 seconds)
query = os.environ.get('QUERY_STRING', '')
wait_seconds = 5

for param in query.split('&'):
    if param.startswith('wait='):
        try:
            wait_seconds = int(param.split('=')[1])
        except:
            wait_seconds = 5

# Cap wait time for safety
wait_seconds = min(wait_seconds, 30)

print("Content-Type: text/html")
print("")

print("<!DOCTYPE html>")
print("<html><body>")
print(f"<h1>Slow CGI Response</h1>")
print(f"<p>Waiting {wait_seconds} seconds...</p>")

# Flush output before waiting
import sys
sys.stdout.flush()

# Wait (this tests server timeout handling)
time.sleep(wait_seconds)

print(f"<p>Done waiting!</p>")
print("</body></html>")
