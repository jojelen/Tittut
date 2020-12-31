client_src = ['client.cpp']
server_src = ['server.cpp']

sdl_dep = dependency('SDL2', required: true)
thread_dep = dependency('threads', required: true)

executable('client', client_src,
           cpp_args: cpp_args,
           include_directories: [tittut_inc],
           dependencies: [sdl_dep])

executable('server', server_src,
           cpp_args: [cpp_args, '-pthread'],
           include_directories: [tittut_inc],
           dependencies: [sdl_dep,thread_dep])
