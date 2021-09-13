project('dpdk-tcp', 'c', 'cpp', default_options : ['cpp_std=c++17'])
dpdk = dependency('libdpdk')

incdir = include_directories('queue/include', 'network/include')
tcp_lib_src = [ 'network/src/xy_api.cc', 'network/src/xy_arp.cc', 'network/src/xy_eth.cc', 'network/src/xy_icmp.cc',
                'network/src/xy_init.cc', 'network/src/xy_ip.cc', 'network/src/xy_socks.cc', 'network/src/xy_tcp.cc',
                'network/src/xy_tcp_recv.cc', 'network/src/xy_tcp_send.cc', 'network/src/xy_epoll.cc', 'network/src/xy_manage.cc'
                'queue/src/xy_syn_api.cc', 'queue/src/xy_syn_socks.cc', ]

tcp_lib = shared_library('tcp_lib',
                        tcp_lib_src,
                        include_directories : incdir,
                        dependencies : dpdk,
                        install : true)


executable('dpdk-try-tcp', sources: 'test/try_tcp.cc', include_directories : incdir, link_with : tcp_lib)
executable('dpdk-hello', sources: 'test/hello.cc', include_directories : incdir, link_with : tcp_lib)