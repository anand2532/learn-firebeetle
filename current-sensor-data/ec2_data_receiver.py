#!/usr/bin/env python3
import json
import os
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HOST = "0.0.0.0"
PORT = 5000
DATA_DIR = os.path.join(os.path.dirname(__file__), "incoming_data")
DATA_FILE = os.path.join(DATA_DIR, "sensor_readings.jsonl")


class IngestHandler(BaseHTTPRequestHandler):
    def _write_json(self, status_code: int, payload: dict) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        if self.path == "/health":
            self._write_json(200, {"status": "ok"})
            return
        self._write_json(404, {"error": "not_found"})

    def do_POST(self) -> None:
        if self.path != "/ingest":
            self._write_json(404, {"error": "not_found"})
            return

        try:
            content_length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self._write_json(400, {"error": "invalid_content_length"})
            return

        raw = self.rfile.read(content_length)
        try:
            payload = json.loads(raw.decode("utf-8"))
        except json.JSONDecodeError:
            self._write_json(400, {"error": "invalid_json"})
            return

        record = {
            "received_at": datetime.now(timezone.utc).isoformat(),
            "source_ip": self.client_address[0],
            "payload": payload,
        }

        os.makedirs(DATA_DIR, exist_ok=True)
        with open(DATA_FILE, "a", encoding="utf-8") as fp:
            fp.write(json.dumps(record) + "\n")

        self._write_json(200, {"saved": True})

    def log_message(self, fmt: str, *args) -> None:
        print(f"[{self.log_date_time_string()}] {self.client_address[0]} {fmt % args}")


def main() -> None:
    os.makedirs(DATA_DIR, exist_ok=True)
    server = ThreadingHTTPServer((HOST, PORT), IngestHandler)
    print(f"Listening on http://{HOST}:{PORT}")
    print(f"Saving records to {DATA_FILE}")
    server.serve_forever()


if __name__ == "__main__":
    main()
