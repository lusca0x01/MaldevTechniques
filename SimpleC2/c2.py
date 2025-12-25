from db import init_db
from server.main import run

if __name__ == "__main__":
    init_db()
    run()
