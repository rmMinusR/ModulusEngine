from functools import reduce
import logging
import zlib
import os.path

# Add custom log level

LOG_USER_LEVEL = 100
logging.addLevelName(LOG_USER_LEVEL, "USER")
def log_user_message(self, message, *args, **kws):
    if self.isEnabledFor(LOG_USER_LEVEL): self._log(LOG_USER_LEVEL, message, args, **kws) 
logging.Logger.user = log_user_message


# Add custom formatting

formatter = logging.Formatter(fmt="%(levelname)-10s %(message)s")

consoleHandler = logging.StreamHandler()
consoleHandler.setFormatter(formatter)

global logger
logger = logging.Logger("STIX")
logger.setLevel("INFO")
logger.addHandler(consoleHandler)


# Version mismatch detection hash
def stable_hash(file):
    with open(file, "r") as thisFile:
        thisFileContent = "".join(thisFile.readlines())
        _hash = zlib.adler32(thisFileContent.encode("utf-8"))
        del thisFileContent
    return _hash
def batch_stable_hash(files:list[str]): return reduce(lambda a,b: (a*33+b)&0xffffffff, [stable_hash( os.path.join(os.path.dirname(__file__), f) ) for f in files], 5381)

tooling_files = ["config.py", "cpp_concepts.py", "rttigen.py", "source_discovery.py", "timings.py"]
version_hash = batch_stable_hash(tooling_files)

tooling_files_v2_ast = ["config.py", "cx_ast.py", "cx_ast_tooling.py", "source_discovery.py", "timings.py"]
ast_version_hash = batch_stable_hash(tooling_files_v2_ast)
