#!/usr/bin/env python3
import sys
import os

def main():
	# Read request body
	content_length = os.environ.get("CONTENT_LENGTH", "0")
	body = ""
	if content_length and content_length != "0":
		body = sys.stdin.read(int(content_length))
	
	content_type = os.environ.get("CONTENT_TYPE", "text/html")
	
	# Output CGI response with proper CRLF
	sys.stdout.write("Content-Type: text/html\r\n")
	sys.stdout.write("\r\n")
	sys.stdout.write(f"<h1>Python CGI OK</h1> <p>Received body: {body}</p>")

if __name__ == '__main__':
	main()
