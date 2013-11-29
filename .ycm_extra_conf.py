import subprocess
import re

def FlagsForFile( filename, **kwargs ):
    make_run = subprocess.Popen(['make', '-pn'], stdout=subprocess.PIPE)
    parser = re.compile(r'CFLAGS .?= (.*?) *$')
    for line in make_run.stdout:
        match = parser.match(line)
        if match:
            break
    make_run.kill()
    flags = re.split(' +', match.group(1))
    flags.extend(['-x', 'c++'])
    return {
        'flags': flags,
        'do_cache': True
    }
