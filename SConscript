
from building import *

cwd     = GetCurrentDir()
path    = [cwd + '/inc', cwd + '/src']
src     = Glob('src/*.c')

group = DefineGroup('dict', src, depend = ['PKG_USING_DICT'], CPPPATH = path)
Return('group')
