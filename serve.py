#!/usr/bin/env python3
"""Tiny web server: serves the UI and runs ./tpdemo for each request.
Usage: python3 serve.py [port]   (default 8000)"""
import json, os, subprocess, sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

ROOT = os.path.dirname(os.path.abspath(__file__))
LIMITS = {  # name: (default, min, max)
    "workers": (4, 1, 16), "queue": (6, 1, 32), "tasks": (20, 1, 200),
    "min": (200, 1, 5000), "max": (800, 1, 5000), "interval": (100, 0, 2000),
}

class Handler(BaseHTTPRequestHandler):
    def _send(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        url = urlparse(self.path)
        if url.path == "/":
            with open(os.path.join(ROOT, "ui", "index.html"), "rb") as f:
                return self._send(200, f.read(), "text/html; charset=utf-8")
        if url.path == "/api/run":
            q = parse_qs(url.query)
            vals = {}
            for k, (d, lo, hi) in LIMITS.items():
                try:
                    v = int(q.get(k, [d])[0])
                except ValueError:
                    return self._send(400, b'{"error":"%s must be an integer"}' % k.encode(), "application/json")
                if not lo <= v <= hi:
                    return self._send(400, ('{"error":"%s must be between %d and %d"}' % (k, lo, hi)).encode(), "application/json")
                vals[k] = v
            if vals["max"] < vals["min"]:
                return self._send(400, b'{"error":"max must be >= min"}', "application/json")
            args = [os.path.join(ROOT, "tpdemo")] + [str(vals[k]) for k in ("workers", "queue", "tasks", "min", "max", "interval")]
            try:
                out = subprocess.run(args, capture_output=True, timeout=120, check=True).stdout
            except FileNotFoundError:
                return self._send(500, b'{"error":"tpdemo not built. Run: make"}', "application/json")
            except Exception as e:
                return self._send(500, json.dumps({"error": str(e)}).encode(), "application/json")
            return self._send(200, out, "application/json")
        self._send(404, b"not found", "text/plain")

    def log_message(self, *a):
        pass

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    print(f"Open http://localhost:{port}")
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()
