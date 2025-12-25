from fastapi import FastAPI
from uvicorn import run
from listener.routes import listener_start


def run_listener(port: int, name: str) -> None:
    app = FastAPI()
    app.include_router(listener_start(name))
    run(app, host="127.0.0.1", port=port, log_level="warning")

if __name__ == "__main__":
    run_listener(8080, "a")