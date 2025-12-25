from models import Task

import sqlite3
from datetime import datetime, timezone
import logging

DB_NAME = "c2.db"
logger = logging.getLogger(__name__)


def init_db():
    try:
        with sqlite3.connect(DB_NAME) as conn:
            c = conn.cursor()

            c.execute("""
            CREATE TABLE IF NOT EXISTS listeners (
                name TEXT PRIMARY KEY,
                port INTEGER,
                created_at TEXT
            )""")

            c.execute("""
            CREATE TABLE IF NOT EXISTS agents (
                name TEXT PRIMARY KEY,
                listener_name TEXT,
                ip TEXT,
                hostname TEXT,
                arrived_at TEXT,
                FOREIGN KEY(listener_name) REFERENCES listeners(name)
            )""")

            c.execute("""
            CREATE TABLE IF NOT EXISTS tasks (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                agent_name TEXT,
                command TEXT,
                issued_at TEXT,
                status TEXT,
                FOREIGN KEY(agent_name) REFERENCES agents(name)
            )
            """)

            c.execute("""
            CREATE TABLE IF NOT EXISTS results (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                task_id INTEGER,
                agent_name TEXT,
                output TEXT,
                received_at TEXT,
                FOREIGN KEY(task_id) REFERENCES tasks(id)
            )
            """)

            conn.commit()
    except sqlite3.Error as e:
        logger.error(f"Database initialization failed: {e}")
        raise


def insert_listener(name: str, port: int) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.execute("INSERT INTO listeners VALUES (?, ?, ?)",
                         (name, port, datetime.now(timezone.utc)))
            conn.commit()
        return True
    except sqlite3.IntegrityError:
        logger.warning(f"Listener '{name}' already exists")
        return False
    except sqlite3.Error as e:
        logger.error(f"Failed to insert listener: {e}")
        return False


def delete_listener(name: str) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.execute("DELETE FROM listeners WHERE name = ?", (name,))
            conn.commit()
        return True
    except sqlite3.Error as e:
        logger.error(f"Failed to delete listener: {e}")
        return False


def insert_agent(name: str, listener_name: str, ip: str, hostname: str) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.execute("INSERT INTO agents VALUES (?, ?, ?, ?, ?)",
                         (name, listener_name, ip, hostname, datetime.now(timezone.utc)))
            conn.commit()
        return True
    except sqlite3.IntegrityError:
        logger.warning(f"Agent '{name}' already exists")
        return False
    except sqlite3.Error as e:
        logger.error(f"Failed to insert agent: {e}")
        return False


def delete_agent(name: str) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.execute("DELETE FROM agents WHERE name = ?", (name,))
            conn.commit()
        return True
    except sqlite3.Error as e:
        logger.error(f"Failed to delete agent: {e}")
        return False


def get_all_agents() -> list:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.row_factory = sqlite3.Row
            return conn.execute("SELECT * FROM agents").fetchall()
    except sqlite3.Error as e:
        logger.error(f"Failed to get agents: {e}")
        return []


def get_all_listener_agents(listener_name: str) -> list:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.row_factory = sqlite3.Row
            return conn.execute("SELECT * FROM agents WHERE listener_name = ?", (listener_name,)).fetchall()
    except sqlite3.Error as e:
        logger.error(f"Failed to get listener agents: {e}")
        return []


def insert_task(agent_name: str, command: str) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            issued_at = datetime.now(timezone.utc)
            conn.execute("""
                INSERT INTO tasks (agent_name, command, issued_at, status)
                VALUES (?, ?, ?, ?)
            """, (agent_name, command, issued_at, "pending"))
            conn.commit()
        return True
    except sqlite3.Error as e:
        logger.error(f"Failed to insert task: {e}")
        return False


def get_pending_task(agent_name: str):
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.row_factory = sqlite3.Row
            task = conn.execute("""
                SELECT * FROM tasks WHERE agent_name = ? AND status = 'pending' ORDER BY issued_at ASC LIMIT 1
            """, (agent_name,)).fetchone()
            return Task(**dict(task)) if task else None
    except sqlite3.Error as e:
        logger.error(f"Failed to get pending task: {e}")
        return None


def mark_task_completed(task_id: int) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.execute("""
                UPDATE tasks SET status = 'completed' WHERE id = ?
            """, (task_id,))
            conn.commit()
        return True
    except sqlite3.Error as e:
        logger.error(f"Failed to mark task completed: {e}")
        return False


def delete_agent_tasks(name: str) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            conn.execute("DELETE FROM tasks WHERE agent_name = ?", (name,))
            conn.commit()
        return True
    except sqlite3.Error as e:
        logger.error(f"Failed to delete agent tasks: {e}")
        return False


def insert_result(task_id: int, agent_name: str, output: str) -> bool:
    try:
        with sqlite3.connect(DB_NAME) as conn:
            received_at = datetime.now(timezone.utc)
            conn.execute("""
                INSERT INTO results (task_id, agent_name, output, received_at)
                VALUES (?, ?, ?, ?)
            """, (task_id, agent_name, output, received_at))
            conn.commit()
        return True
    except sqlite3.Error as e:
        logger.error(f"Failed to insert result: {e}")
        return False
