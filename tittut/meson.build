client_src = ['client.cpp']
server_src = ['server.cpp']

sdl_lib = dependency('SDL2', required: true)

executable('client', client_src,
           cpp_args: cpp_args,
           include_directories: [tittut_inc],
           dependencies: sdl_lib)

executable('server', server_src,
           cpp_args: cpp_args,
           include_directories: [tittut_inc])
