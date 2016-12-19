all:
	g++ map_server.cpp -o map_server -levent
	gcc event_client.c -o event_client -lpthread
	/home/work/7628/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc event_client.c -o event_clientl -lpthread
