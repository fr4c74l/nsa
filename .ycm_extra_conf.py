_flags = [
'-Wall',
'-Wextra',
'-std=c++11',
'-x',
'c++',
]

def FlagsForFile( filename, **kwargs ):
    return {
        'flags': _flags,
        'do_cache': True
    }
