#!/usr/bin/env python3

import subprocess
import sys
import time
import signal
import requests
import os

WEB_SERV_PORT = 4444
NGINX_PORT = 1234

TEST_PATHS = [
    "/",
    "/index.html",
    "/login",
    "/notfound",
]

WEB_SERV_BIN = "./webserv"   # change if needed
NGINX_BIN = "nginx"


def start_webserv(conf):
    print("[+] Starting webserv...")
    return subprocess.Popen(
        [WEB_SERV_BIN, conf],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )


def start_nginx(conf):
    print("[+] Starting nginx...")
    return subprocess.Popen(
        [NGINX_BIN, "-c", conf, "-g", f"daemon off;"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )


def stop_process(proc, name):
    if not proc:
        return
    print(f"[+] Stopping {name}...")
    try:
        proc.send_signal(signal.SIGTERM)
        proc.wait(timeout=3)
    except Exception:
        proc.kill()


def fetch_status(base_url, path):
    try:
        r = requests.get(base_url + path, timeout=3)
        return r.status_code
    except Exception:
        return "ERR"


def compare():
    print("\n=== Comparing Responses ===")

    web_base = f"http://localhost:{WEB_SERV_PORT}"
    nginx_base = f"http://localhost:{NGINX_PORT}"

    mismatch = False

    for path in TEST_PATHS:
        web_status = fetch_status(web_base, path)
        nginx_status = fetch_status(nginx_base, path)

        if web_status != nginx_status:
            mismatch = True
            print(f"[X] MISMATCH {path}")
            print(f"    webserv: {web_status}")
            print(f"    nginx  : {nginx_status}")
        else:
            print(f"[OK] {path} -> {web_status}")

    if not mismatch:
        print("\n✔ All status codes match")
    else:
        print("\n✖ Differences detected")


def main():
    if len(sys.argv) != 3:
        print("Usage:")
        print("./tester.py <webserv.conf> <nginx.conf>")
        sys.exit(1)
    path = os.getcwd()
    web_conf = os.path.join(path, sys.argv[1])
    nginx_conf = os.path.join(path, sys.argv[2])

    web_proc = None
    nginx_proc = None

    try:
        web_proc = start_webserv(web_conf)
        time.sleep(2)

        nginx_proc = start_nginx(nginx_conf)
        time.sleep(2)

        compare()

    finally:
        stop_process(web_proc, "webserv")
        stop_process(nginx_proc, "nginx")


if __name__ == "__main__":
    main()
