import time
import os

def main():
    i = 0
    key = "STARTED_BY"
    with open("jojo", "w") as f:
        f.write("umask test")


    stat = os.stat("jojo")
    permissions = oct(stat.st_mode & 0o777)
    print(f"joj0 permissions: {permissions}")
    for name, value in os.environ.items():
        print(f"{name}={value}")
    while 1:
        time.sleep(3)
        print(f"HELLO FROM {os.getenv(key)} {i},-------------> {os.getpid()}", flush=True)
        i+=1

if __name__ == "__main__":
    main()

