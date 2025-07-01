
from taskmaster.supervisor import Supervisor
from taskmaster.shell import start_shell
import threading
import time
from taskmaster.loger import setup_logger

def monitor_loop(supervisor):
    """Background thread to monitor processes"""
    while True:
        supervisor.monitor_processes()
        time.sleep(1)

if __name__ == "__main__":
    setup_logger()
    supervisor = Supervisor("config.yaml")
    supervisor.start_all()
    
    monitor_thread = threading.Thread(target=monitor_loop, args=(supervisor,), daemon=True)
    monitor_thread.start()
    
    try:
        start_shell(supervisor)
    except KeyboardInterrupt:
        print("\nShutting down...")
        supervisor.stop_all()