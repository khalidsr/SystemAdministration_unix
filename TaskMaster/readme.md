# TaskMaster - Process Supervisor

TaskMaster is a lightweight process supervisor that manages and monitors child processes according to a configuration file. It provides:

## Key Features

✅ **Process Management**  
- Start/stop/restart processes  
- Automatic restart policies (always, never, on unexpected exit)  
- Multiple process instances per program  

✅ **Configuration**  
- YAML configuration file  
- Runtime configuration reload (SIGHUP)  
- Per-program settings (env vars, working dir, umask, etc.)  

✅ **Monitoring & Reliability**  
- Continuous process health checks  
- Startup success detection  
- Restart attempt limits  

✅ **User Interface**  
- Interactive control shell  
- tab completion  
- Real-time status monitoring  

✅ **Logging**  
- Detailed event logging  
- Log rotation  
- Both file and console output  

