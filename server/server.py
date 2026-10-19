"""
API sederhana penerima data Water Level Monitoring.

Instal : pip install fastapi uvicorn
Jalankan: uvicorn server:app --host 0.0.0.0 --port 8000
Docs    : http://localhost:8000/docs
"""
import os
import sqlite3
from datetime import datetime, timezone
from typing import Literal, Optional

from fastapi import FastAPI, Header, HTTPException
from pydantic import BaseModel

API_KEY = os.getenv("WATER_API_KEY", "ganti-dengan-kunci-acak-milikmu")
DB_FILE = os.getenv("WATER_DB_FILE", "water_level.db")

app = FastAPI(title="Water Level Monitoring API")


class Reading(BaseModel):
    device_id: str
    distance_cm: Optional[float] = None        # null jika sensor error
    water_level_cm: Optional[float] = None     # null jika sensor error
    status: Literal["AMAN", "WASPADA", "BAHAYA", "SENSOR_ERROR"]
    uptime_ms: Optional[int] = None


def get_conn():
    return sqlite3.connect(DB_FILE)


@app.on_event("startup")
def init_db():
    conn = get_conn()
    with conn:
        conn.execute(
            """CREATE TABLE IF NOT EXISTS readings (
                   id INTEGER PRIMARY KEY AUTOINCREMENT,
                   received_at TEXT NOT NULL,
                   device_id TEXT NOT NULL,
                   distance_cm REAL,
                   water_level_cm REAL,
                   status TEXT NOT NULL,
                   uptime_ms INTEGER
               )"""
        )
    conn.close()


@app.post("/api/water-level")
def receive(reading: Reading, x_api_key: str = Header(default="")):
    if x_api_key != API_KEY:
        raise HTTPException(status_code=401, detail="API key salah")

    # Waktu diberikan server karena ESP32 belum memakai NTP
    received_at = datetime.now(timezone.utc).isoformat()

    conn = get_conn()
    with conn:
        conn.execute(
            "INSERT INTO readings (received_at, device_id, distance_cm, "
            "water_level_cm, status, uptime_ms) VALUES (?, ?, ?, ?, ?, ?)",
            (received_at, reading.device_id, reading.distance_cm,
             reading.water_level_cm, reading.status, reading.uptime_ms),
        )
    conn.close()
    return {"ok": True, "received_at": received_at}


@app.get("/api/water-level/latest")
def latest(limit: int = 10):
    conn = get_conn()
    conn.row_factory = sqlite3.Row
    rows = conn.execute(
        "SELECT * FROM readings ORDER BY id DESC LIMIT ?", (min(limit, 200),)
    ).fetchall()
    conn.close()
    return [dict(r) for r in rows]
