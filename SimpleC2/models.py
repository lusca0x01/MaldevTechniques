from pydantic import BaseModel


class BaseAgent(BaseModel):
    hostname: str
    os: str


class AgentRegister(BaseAgent):
    pass


class Agent(BaseAgent):
    name: str
    listener_name: str
    ip: str


class Task(BaseModel):
    id: int
    agent_name: str
    command: str
    issued_at: str
    status: str


class Result(BaseModel):
    task_id: int
    agent_name: str
    output: str
