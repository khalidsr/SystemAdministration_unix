import cmd


class TaskMasterShell(cmd.Cmd):
    intro = "TaskMaster control shell. Type 'help' or '?' for commands."
    prompt = "taskmaster> "

    def __init__(self, supervisor):
        super().__init__()
        self.supervisor = supervisor

    def do_status(self, arg):
        """Show status of all programs"""
        status = self.supervisor.get_status()
        for name, procs in status.items():
            print(f"{name}:")
            for proc in procs:
                print(f"  Process {proc['index']}:")
                print(f"    State: {proc['state']}")
                print(f"    PID: {proc['pid']}")
                if proc['state'] == "RUNNING":
                    print(f"    Uptime: {proc['uptime']:.2f}s")
                print(f"    Restart attempts: {proc['attempts']}")

    def do_start(self, name):
        """Start a program: start <name>"""
        if not name:
            print("Usage: start <program_name>")
            return
        
        if name not in self.supervisor.config["programs"]:
            print(f"Error: Unknown program '{name}'")
            return
        
        self.supervisor.spawn_process(name)

    def do_stop(self, name):
        """Stop a program: stop <name>"""
        if not name:
            print("Usage: stop <program_name>")
            return
        
        if name not in self.supervisor.processes:
            print(f"Error: Program '{name}' not running")
            return
        
        for i in range(len(self.supervisor.processes[name])):
            self.supervisor.stop_process(name, i)

    def do_restart(self, name):
        """Restart a program: restart <name>"""
        if not name:
            print("Usage: restart <program_name>")
            return
        
        if name not in self.supervisor.processes:
            print(f"Error: Program '{name}' not running")
            return
        
        for i in range(len(self.supervisor.processes[name])):
            self.supervisor.restart_process(name, i)

    def do_reload(self, arg):
        """Reload configuration file"""
        self.supervisor.reload_config()
        print("Configuration reloaded")

    def do_exit(self, arg):
        """Exit the TaskMaster shell"""
        print("Shutting down all processes...")
        self.supervisor.stop_all()
        return True

    def do_quit(self, arg):
        """Exit the TaskMaster shell"""
        return self.do_exit(arg)

    def emptyline(self):
        pass

    def postloop(self):
        print("TaskMaster shutdown complete")
        
    def do_help(self, arg):
        return super().do_help(arg)

    def do_help(self, arg):
        """Custom help command."""
        commands = {
            'status': "Show status of all programs",
            'start': "Start a program: start <program_name>",
            'stop': "Stop a program: stop <program_name>",
            'restart': "Restart a program: restart <program_name>",
            'reload': "Reload the configuration file",
            'exit': "Exit the TaskMaster shell",
            'quit': "Same as 'exit'",
            'help': "Show this help message"
        }
        if arg:
                arg = arg.strip()
                if arg in commands:
                    print(f"{arg}: {commands[arg]}")
                else:
                    print(f"No help available for '{arg}'")
        else:
            print("Available commands:")
            for cmd, desc in commands.items():
                print(f"  {cmd:10} - {desc}")

def start_shell(supervisor):
    TaskMasterShell(supervisor).cmdloop()