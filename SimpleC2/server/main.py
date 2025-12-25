import cmd
from server.listeners_manager import launch_listener, kill_listener, shutdown_all, check_listeners
from server.agents_manager import check_if_agent, check_agents, delete_tasks_from_agent, quit_agent, drop_file_to_agent, agent_exec_command, remove_all
from server.executors_manager import gen_executor


class AgentMode(cmd.Cmd):
    def __init__(self, agent_name=None):
        super().__init__()
        if not agent_name:
            raise ValueError("AgentMode requires a valid agent name.")
        self.agent_name = agent_name
        self.prompt = f"(agent:{agent_name}) "

    intro = "Agent mode. Type ? to list commands.\n"

    def do_download(self, arg):
        "Send a download task to this agent: download <filename>"
        file = arg.strip()
        if not file:
            print("Usage: download <filename>")
            return
        drop_file_to_agent(self.agent_name, file)
        print(f"Download task for '{file}' sent to '{self.agent_name}'.")

    def do_exec(self, arg):
        "Send a command to be executed by this agent: exec <command>"
        command = arg.strip()
        if not command:
            print("Usage: exec <command>")
            return
        agent_exec_command(self.agent_name, command)
        print(f"Exec task sent to '{self.agent_name}': {command}")

    def do_clear_tasks(self, arg):
        "Clear all tasks for this agent"
        delete_tasks_from_agent(self.agent_name)

    def do_exit(self, arg):
        print("Exiting agent mode.")
        return True

    def do_quit(self, arg):
        "Signal this agent to quit"
        resp = input(
            "Are you sure you want to quit this agent? (y/n): ").strip().lower()
        if resp == "y":
            try:
                quit_agent(self.agent_name)
                print(f"Agent '{self.agent_name}' signaled to quit.")
                return True
            except Exception as e:
                print(f"Failed to quit agent: {e}")
        else:
            print("Action canceled.")

    def emptyline(self):
        pass


class BeaconMode(cmd.Cmd):
    def do_exit(self, arg):
        "Exit beacon mode"
        print("Exiting beacon mode.")
        return True

    def emptyline(self):
        pass


class ListenerMode(cmd.Cmd):
    "Start a listener: start <port>"

    intro = "Listener mode. Type ? to list commands.\n"
    prompt = "(listener) "

    def do_start(self, arg):
        "Start a listener: start <name>"
        try:
            port = int(arg.strip())
            launch_listener(port)
        except ValueError:
            print("Invalid port number")
        except Exception as e:
            print(f"Exception: {e}")

    def do_stop(self, arg):
        "Stop a listener: stop <name>"
        name = arg.strip()
        if name:
            kill_listener(name)
        else:
            print("You must specify a listener name.")

    def do_exit(self, arg):
        "Exit listener mode"
        print("Exiting listener mode.")
        return True

    def emptyline(self):
        pass


class C2Menu(cmd.Cmd):
    intro = "C2 CLI. Type ? to list commands.\n"
    prompt = "(c2) "

    def do_agent(self, arg):
        "Enter agent mode: agent [agent_name]"
        name = arg.strip()

        if not name:
            print(f"You need to pass agent [agent_name]")
            return

        if not check_if_agent(name):
            print(f"Agent '{name}' not found.")
            return

        AgentMode(agent_name=name).cmdloop()

    def do_listener(self, arg):
        "Enter listener mode"
        ListenerMode().cmdloop()

    def do_shutdown(self, arg):
        "Kill all running listeners"
        confirm = input(
            "Are you sure you want to shut down all listeners? (y/n): ").strip().lower()
        if confirm == "y":
            shutdown_all()
            print("All listeners shut down.")
        else:
            print("Shutdown canceled.")

    def do_list(self, arg):
        "List entities: list agents | list listeners"
        type = arg.strip().lower()

        if type == "agents":
            check_agents()
        elif type == "listeners":
            check_listeners()
        elif not type:
            print("Usage: list <agents|listeners>")
        else:
            print(
                f"Unknown list type: '{type}'. Use 'agents' or 'listeners'.")

    def do_payload(self, arg):
        "Generate an agent payload: payload"

        ip = input("Listener IP [127.0.0.1]: ").strip() or "127.0.0.1"
        port = input("Listener Port [4444]: ").strip() or "4444"
        arch = input("Architecture (x32/x64) [x64]: ").strip() or "x64"
        name = input("Output name: ").strip() or "agent"
        os_type = input("Desired OS (win, linux): ") or "win"

        try:
            port = int(port)
        except ValueError:
            print("Invalid port number.")
            return

        if arch not in ["x64", "x32", "x86"]:
            print("Architecture must be 'x64', 'x86' or 'x32'")
            return

        if os_type not in ["win", "linux"]:
            print("OS type must be 'win' or 'linux'")
            return

        try:
            gen_executor(ip, port, arch, name, os_type)
            print(f"Payload '{name}' generated successfully.")
        except Exception as e:
            print(f"Payload generation failed: {e}")

    def do_exit(self, arg):
        "Exit the CLI (kills all listeners)"
        self.do_shutdown(arg)
        print("See ya!")
        return True

    def emptyline(self):
        pass


def run():
    try:
        C2Menu().cmdloop()
    except KeyboardInterrupt:
        remove_all()
        shutdown_all()


if __name__ == "__main__":
    run()
