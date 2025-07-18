### MattDaemon - System Monitoring Daemon
# Description
MattDaemon is a lightweight Linux daemon that runs in the background, monitoring system activity and handling client connections. It provides a simple TCP server that allows authorized clients to send commands and receive responses.

# Features:
- Runs as a background daemon (detached from terminal)

- Manages a single instance using a lockfile (/var/lock/matt_daemon.lock)

- Listens on port 4242 (configurable) for incoming connections

- Supports up to 3 concurrent clients

- Logs all activities with timestamps

- Gracefully handles SIGINT, SIGTERM, and SIGHUP signals

- Client command processing (quit to shut down the daemon)

# Installation & Usage
1.  Build the Project
    ```
      make
    ```
3. Run the Daemon
    ```
      sudo ./matt_daemon
    ```
3. Connect a Client
  Use netcat (nc) to connect:
  ```
    nc localhost 4242
  ```
Send commands (e.g., hello, quit)

Type quit to shut down the daemon

4. Check Logs

Logs are printed in /var/log/matt_daemon/matt_daemon.log.
