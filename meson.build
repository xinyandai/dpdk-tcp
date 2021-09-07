project('dpdk-tcp', 'c', 'cpp', default_options : ['cpp_std=c++17'])
dpdk = dependency('libdpdk')

incdir = include_directories('queue', 'network')
tcp_lib_src = ['src/xy_api.cc', 'src/xy_arp.cc', 'src/xy_eth.cc', 'src/xy_icmp.cc', 'src/xy_init.cc', 'src/xy_ip.cc', 'src/xy_socks.cc', 'src/xy_syn_api.cc', 'src/xy_syn_socks.cc', 'src/xy_tcp.cc', 'src/xy_tcp_recv.cc', 'src/xy_tcp_send.cc']


tcp_lib = shared_library('tcp_lib',
                        tcp_lib_src,
                        include_directories : incdir,
                        dependencies : dpdk,
                        install : true)


executable('dpdk-try-tcp', sources: 'test/try_tcp.cc', include_directories : incdir, link_with : tcp_lib)
executable('dpdk-hello', sources: 'test/hello.cc', include_directories : incdir, link_with : tcp_lib)