#!/usr/bin/env python3
import sys
import os


def main():
	body = sys.stdin.read()
	content_type = os.environ.get("CONTENT_TYPE", "text/html")
	print(f"Content-Type: {content_type}\r\n\r\n<h1>Python CGI OK</h1> <p>Received body: {body}</p>")

if __name__ == '__main__':
	main()
