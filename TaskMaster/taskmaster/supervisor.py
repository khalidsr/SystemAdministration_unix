import yaml
import subprocess
import os
import signal
import time
import logging
from typing import Dict, List, Any
import shlex
import io

class Supervisor:
    def __init__(self, config_path: str):
        self.config_path = config_path
        self.config = self.load_config(config_path)
        self.processes: Dict[str, List[Dict[str, Any]]] = {}
        self.logger = logging.getLogger('taskmaster')
        self.setup_signal_handlers()

    def load_config(self, path: str) -> Dict:
        """Load and validate the YAML config file."""
        with open(path) as f:
            config = yaml.safe_load(f)
        
        if "programs" not in config:
            raise ValueError("Config must contain 'programs' section")
        for name, prog in config["programs"].items():
            if isinstance(prog.get("exitcodes"), int):
                prog["exitcodes"] = [prog["exitcodes"]]
            elif "exitcodes" not in prog:
                prog["exitcodes"] = [0]   

            prog.setdefault("numprocs", 1)
            prog.setdefault("autostart", False)
            prog.setdefault("restart", "unexpected")
            prog.setdefault("startsecs", 1)
            prog.setdefault("startretries", 3)
            prog.setdefault("stopsignal", "TERM")
            prog.setdefault("stoptime", 10)
            prog.setdefault("umask", None)
            prog.setdefault("workingdir", ".")
            prog.setdefault("env", {})
            
        return config
    
    def setup_signal_handlers(self):
        signal.signal(signal.SIGHUP, self.handle_sighup)

    def handle_sighup(self, signum, frame):
        self.logger.info("Received SIGHUP, reloading configuration")
        self.reload_config()

    def reload_config(self):
        """Reload configuration while preserving unchanged processes"""
        old_config = self.config
        new_config = self.load_config(self.config_path)
        for name in list(self.processes.keys()):
            if name not in new_config["programs"]:
                self.logger.info(f"Stopping removed program: {name}")
                for i in range(len(self.processes[name])):
                    self.stop_process(name, i)
                del self.processes[name]

        for name, new_prog_config in new_config["programs"].items():
            if name in old_config["programs"]:
                if old_config["programs"][name] == new_prog_config:
                    continue

                self.logger.info(f"Updating configuration for program: {name}")

                for i in range(len(self.processes.get(name, []))):
                    self.stop_process(name, i)

                for _ in range(new_prog_config.get("numprocs", 1)):
                    self.spawn_process(name)

        
        for name, new_prog_config in new_config["programs"].items():
            if name not in old_config["programs"] and new_prog_config.get("autostart", False):
                self.logger.info(f"Starting new program: {name}")
                for _ in range(new_prog_config.get("numprocs", 1)):
                    self.spawn_process(name)
    
        self.config = new_config


    def setup_process_environment(self, program_config: Dict) -> Dict:
        """Prepare environment variables for subprocess."""
        
        env = os.environ.copy()
        for k, v in program_config["env"].items():
            env[str(k)] = str(v)
        return env

    def start_all(self):
        """Start all programs marked with autostart=True."""
        for name, cfg in self.config["programs"].items():
            if cfg.get("autostart", False):
                for _ in range(cfg.get("numprocs")):
                    self.spawn_process(name)

    def monitor_processes(self):
        """Check all processes and restart if needed."""
        for name, procs in list(self.processes.items()):
            for i, proc_info in enumerate(procs[:]):
                if proc_info["popen"] is None:
                    continue

                returncode = proc_info["popen"].poll()
                if returncode is not None:
                    self.logger.info(f"Process {name}[{i}] (PID {proc_info['popen'].pid}) died with code {returncode}")

                    cfg = proc_info["config"]
                    should_restart = False

                    if cfg["restart"] == "always":
                        should_restart = True
                    elif cfg["restart"] == "unexpected" and returncode not in cfg["exitcodes"]:
                        should_restart = True

                    if should_restart and proc_info["attempts"] < cfg["startretries"]:
                        proc_info["attempts"] += 1
                        self.logger.info(f"Restarting {name}[{i}] (attempt {proc_info['attempts']}/{cfg['startretries']})")

                        new_proc_info = self.spawn_process(name, return_info=True)
                        if new_proc_info:
                            new_proc_info["attempts"] = proc_info["attempts"]
                            self.processes[name][i] = new_proc_info
                    else:
                        self.logger.info(f"Not restarting {name}[{i}]")

                        if isinstance(proc_info.get("stdout"), io.IOBase):
                            proc_info["stdout"].close()
                        if isinstance(proc_info.get("stderr"), io.IOBase):
                            proc_info["stderr"].close()

                        # proc_info["stdout"].close()
                        # proc_info["stderr"].close()
                        proc_info["popen"] = None
                        proc_info["state"] = "EXITED"



    def spawn_process(self, name: str, return_info: bool = False):
        """Spawn a new process for the given program."""
        if name not in self.config["programs"]:
            raise ValueError(f"Unknown program: {name}")

        cfg = self.config["programs"][name]

        process_info = {
            "attempts": 0,
            "start_time": None,
            "popen": None,
            "config": cfg,
            "state": "STARTING"
        }

        old_umask = None
        try:
            stdout = subprocess.DEVNULL
            stderr = subprocess.DEVNULL
            if cfg.get("stdout"):
                stdout = open(cfg["stdout"], "a+")
            if cfg.get("stderr"):
                stderr = open(cfg["stderr"], "a+")

            env = self.setup_process_environment(cfg)


            
            if cfg["umask"] is not None:
                old_umask =os.umask(cfg["umask"])
            else:
                old_umask = None


            cmd_args = shlex.split(cfg["cmd"])

            proc = subprocess.Popen(
                cmd_args,
                cwd=cfg["workingdir"],
                env=env,
                stdout=stdout,
                stderr=stderr,
                start_new_session=True
            )

            process_info.update({
                "popen": proc,
                "start_time": time.time(),
                "stdout": stdout,
                "stderr": stderr
            })

            if not return_info:
                if name not in self.processes:
                    self.processes[name] = []
                self.processes[name].append(process_info)

            self.logger.info(f"Spawned {name} (PID: {proc.pid})")

            return process_info if return_info else True

        except Exception as e:
            self.logger.error(f"Failed to spawn {name}: {str(e)}")
            return None if return_info else False

        finally:
            if old_umask is not None:
                os.umask(old_umask)


    def stop_process(self, name: str, index: int = 0):
        proc_info = self.processes[name][index]
        if proc_info["popen"] is None:
            return

        try:
            stop_signal = getattr(signal, f"SIG{proc_info['config'].get('stopsignal', 'TERM')}")
            proc_info["popen"].send_signal(stop_signal)
            try:
                proc_info["popen"].wait(timeout=proc_info["config"].get("stoptime", 10))
                self.logger.info(f"Gracefully stopped {name}[{index}] (PID: {proc_info['popen'].pid})")
            except subprocess.TimeoutExpired:
                proc_info["popen"].kill()
                self.logger.info(f"Force killed {name}[{index}] (PID: {proc_info['popen'].pid})")

            if isinstance(proc_info.get("stdout"), io.IOBase):
                proc_info["stdout"].close()
            if isinstance(proc_info.get("stderr"), io.IOBase):
                proc_info["stderr"].close()

            proc_info["popen"] = None
            proc_info["state"] = "STOPPED"

        except Exception as e:
            self.logger.error(f"Error stopping process {name}[{index}]: {str(e)}")


    def stop_all(self):
        """Stop all running processes."""
      
        for name in list(self.processes.keys()):
            for i in range(len(self.processes[name])):
                if self.processes[name][i]["popen"] is not None:
                    self.stop_process(name, i)

    def restart_process(self, name: str, index: int = 0):
        """Restart a specific process instance."""
        self.stop_process(name, index)
        return self.spawn_process(name)

    def get_status(self) -> Dict[str, List[Dict]]:
        """Get current status of all processes."""
        status = {}
        for name, procs in self.processes.items():
            status[name] = []
            for i, p in enumerate(procs):
                state = "STOPPED"
                pid = "N/A"
                uptime = "N/A"
                
                if p["popen"] is not None:
                    if p["popen"].poll() is None:
                        state = "RUNNING"
                        pid = p["popen"].pid
                        uptime = time.time() - p["start_time"]
                    else:
                        state = "EXITED"
                
                status[name].append({
                    "index": i,
                    "state": state,
                    "pid": pid,
                    "uptime": uptime,
                    "attempts": p["attempts"]
                })
        return status