from listener.main import run_listener
from uuid import uuid4
from db import insert_listener, delete_listener, get_all_listener_agents
from server.agents_manager import quit_agent, quit_all_agents
import multiprocessing

listeners = {}


def launch_listener(port: int) -> None:
    name = "listener_" + str(uuid4())
    try:
        p = multiprocessing.Process(target=run_listener, args=(port, name))
        p.start()
        listeners[name] = p
        insert_listener(name, port)
        print(f"Launched {name} on port {port}")
    except Exception as e:
        print(f"Failed to launch listener: {e}")


def kill_listener(name: str):
    agents = get_all_listener_agents(name)
    for agent in agents:
        quit_agent(agent["name"])

    p = listeners.get(name)
    if p and p.is_alive():
        p.terminate()
        p.join()
        listeners.pop(name, None)
        delete_listener(name)
        print(f"Killed listener '{name}' and associated agents.")
    else:
        if name in listeners:
            listeners.pop(name, None)
            delete_listener(name)
        print(f"Listener '{name}' not running or already dead.")


def shutdown_all():
    quit_all_agents()
    for name, p in list(listeners.items()):
        if p and p.is_alive():
            p.terminate()
            p.join()
            delete_listener(name)
            print(f"Killed listener '{name}'")
        else:
            print(f"Listener '{name}' not running or already dead.")
    listeners.clear()
    print("All listeners cleared from memory.")


def check_listeners():
    if not listeners:
        print("No listeners running.")
        return

    print("Current listener status:")
    for name, p in listeners.items():
        status = "alive" if p.is_alive() else "dead"
        print(f" - {name}: {status}")
