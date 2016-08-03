from __future__ import print_function

import syslog

def log(*objects):
    print(*objects)
    for i in objects:
        syslog.syslog(str(i))

