import logging

def setup_logger():
    logger = logging.basicConfig(
        filename="taskmaster.log",
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s"
    )

    return logger