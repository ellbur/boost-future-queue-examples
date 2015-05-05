
import os

flags = [
    '-std=c++11',
    '-x',
    'c++',
    '-I/home/owen/cpp-actors/src',
    '-I../src',
]

def IsHeaderFile(filename):
    extension = os.path.splitext(filename)[ 1 ]
    return extension in [ '.h', '.hxx', '.hpp', '.hh' ]

def GetCompilationInfoForFile(filename):
    return None

def FlagsForFile(filename, **kwargs):
    return {
        'flags': flags,
        'do_cache': True
    }

