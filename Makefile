# WARNING! This file is automatically generated by "rake gen".
# WARNING! Any modification to it would be lost after another "rake gen" operation!

# Automatically generated at Mon May 24 09:02:03 +0800 2010

CC_debug=gcc
CFLAGS_debug=-ggdb -Wall -D_FILE_OFFSET_BITS=64 -Isrc -Ixdk -Itest -I3rdparty 
LDFLAGS_debug=-lpthread -lm
CC_release=gcc
CFLAGS_release=-O3 -pthread -D_FILE_OFFSET_BITS=64 -Isrc -Ixdk -Itest -I3rdparty 
LDFLAGS_release=-lpthread -lm


default: debug

debug: bin-debug obj-debug src-debug 

release: bin-release obj-release src-release 



bin-debug:
	mkdir -p bin-debug

bin-release:
	mkdir -p bin-release



obj-debug:
	mkdir -p obj-debug/3rdparty/vnc_auth
	mkdir -p obj-debug/src
	mkdir -p obj-debug/test
	mkdir -p obj-debug/xdk/3rdparty/bigint
	mkdir -p obj-debug/xdk/3rdparty/crypto/md5
	mkdir -p obj-debug/xdk/3rdparty/crypto/sha1
	mkdir -p obj-debug/xdk/3rdparty/libinotifytools


obj-release:
	mkdir -p obj-release/3rdparty/vnc_auth
	mkdir -p obj-release/src
	mkdir -p obj-release/test
	mkdir -p obj-release/xdk/3rdparty/bigint
	mkdir -p obj-release/xdk/3rdparty/crypto/md5
	mkdir -p obj-release/xdk/3rdparty/crypto/sha1
	mkdir -p obj-release/xdk/3rdparty/libinotifytools




api: 3rdparty/vnc_auth/d3des.h 3rdparty/vnc_auth/d3des.c src/vnc_proxy_ctl.c src/vnc_proxy.c xdk/xhash.h xdk/xmemory.h xdk/xdef.c xdk/xkeepalive.h xdk/xhash.c xdk/xutils.c xdk/xcrypto.h xdk/xoption.h xdk/xlog.h xdk/xsys.c xdk/xnet.c xdk/xstr.c xdk/xconf.h xdk/xoption.c xdk/xvec.h xdk/xconf.c xdk/xcrypto.c xdk/xsys.h xdk/xutils.h xdk/xlog.c xdk/xmemory.c xdk/xnet.h xdk/xvec.c xdk/xkeepalive.c xdk/xbigint.c xdk/xbigint.h xdk/3rdparty/crypto/md5/md5.h xdk/3rdparty/crypto/md5/md5.c xdk/3rdparty/crypto/sha1/sha1.h xdk/3rdparty/crypto/sha1/sha1.c xdk/3rdparty/bigint/bigint.h xdk/3rdparty/bigint/bigint.c xdk/3rdparty/libinotifytools/inotifytools_p.h xdk/3rdparty/libinotifytools/redblack.h xdk/3rdparty/libinotifytools/inotifytools.c xdk/3rdparty/libinotifytools/inotifytools.h xdk/3rdparty/libinotifytools/redblack.c xdk/xstr.h xdk/xdef.h 
	doxygen


clean:
	rm -rf bin-debug bin-release 
	rm -rf obj-debug obj-release 
	rm -rf api
	rm -f *.log
	find . -iname *~ -delete

3rdparty-debug: bin-debug obj-debug obj-debug/3rdparty/vnc_auth/d3des.o

obj-debug/3rdparty/vnc_auth/d3des.o: 3rdparty/vnc_auth/d3des.c 3rdparty/vnc_auth/d3des.h
	$(CC_debug) $(CFLAGS_debug) -c 3rdparty/vnc_auth/d3des.c -o obj-debug/3rdparty/vnc_auth/d3des.o


bin-debug/vnc_proxy: obj-debug/3rdparty/vnc_auth/d3des.o obj-debug/src/vnc_proxy.o obj-debug/xdk/3rdparty/crypto/md5/md5.o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o obj-debug/xdk/xconf.o obj-debug/xdk/xcrypto.o obj-debug/xdk/xdef.o obj-debug/xdk/xhash.o obj-debug/xdk/xlog.o obj-debug/xdk/xmemory.o obj-debug/xdk/xnet.o obj-debug/xdk/xoption.o obj-debug/xdk/xstr.o obj-debug/xdk/xsys.o obj-debug/xdk/xutils.o obj-debug/xdk/xvec.o
	$(CC_debug) $(CFLAGS_debug) $(LDFLAGS_debug) obj-debug/src/vnc_proxy.o obj-debug/3rdparty/vnc_auth/d3des.o obj-debug/xdk/3rdparty/crypto/md5/md5.o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o obj-debug/xdk/xconf.o obj-debug/xdk/xcrypto.o obj-debug/xdk/xdef.o obj-debug/xdk/xhash.o obj-debug/xdk/xlog.o obj-debug/xdk/xmemory.o obj-debug/xdk/xnet.o obj-debug/xdk/xoption.o obj-debug/xdk/xstr.o obj-debug/xdk/xsys.o obj-debug/xdk/xutils.o obj-debug/xdk/xvec.o -o bin-debug/vnc_proxy

bin-debug/vnc_proxy_ctl: obj-debug/src/vnc_proxy_ctl.o obj-debug/xdk/3rdparty/crypto/md5/md5.o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o obj-debug/xdk/xcrypto.o obj-debug/xdk/xdef.o obj-debug/xdk/xmemory.o obj-debug/xdk/xstr.o obj-debug/xdk/xsys.o obj-debug/xdk/xutils.o
	$(CC_debug) $(CFLAGS_debug) $(LDFLAGS_debug) obj-debug/src/vnc_proxy_ctl.o obj-debug/xdk/3rdparty/crypto/md5/md5.o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o obj-debug/xdk/xcrypto.o obj-debug/xdk/xdef.o obj-debug/xdk/xmemory.o obj-debug/xdk/xstr.o obj-debug/xdk/xsys.o obj-debug/xdk/xutils.o -o bin-debug/vnc_proxy_ctl

obj-debug/src/vnc_proxy.o: 3rdparty/vnc_auth/d3des.h src/vnc_proxy.c xdk/xcrypto.h xdk/xlog.h xdk/xmemory.h xdk/xnet.h xdk/xstr.h xdk/xutils.h xdk/xvec.h
	$(CC_debug) $(CFLAGS_debug) -c src/vnc_proxy.c -o obj-debug/src/vnc_proxy.o

obj-debug/src/vnc_proxy_ctl.o: src/vnc_proxy_ctl.c xdk/xdef.h xdk/xmemory.h xdk/xstr.h xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c src/vnc_proxy_ctl.c -o obj-debug/src/vnc_proxy_ctl.o

src-debug: bin-debug obj-debug bin-debug/vnc_proxy bin-debug/vnc_proxy_ctl obj-debug/src/vnc_proxy.o obj-debug/src/vnc_proxy_ctl.o


runtest-debug: test-debug
	@clear
	@echo Running test module \'test\'
	@echo ==========================

test-debug: bin-debug obj-debug 


obj-debug/xdk/3rdparty/bigint/bigint.o: xdk/3rdparty/bigint/bigint.c xdk/3rdparty/bigint/bigint.h xdk/xbigint.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/3rdparty/bigint/bigint.c -o obj-debug/xdk/3rdparty/bigint/bigint.o

obj-debug/xdk/3rdparty/crypto/md5/md5.o: xdk/3rdparty/crypto/md5/md5.c xdk/3rdparty/crypto/md5/md5.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/3rdparty/crypto/md5/md5.c -o obj-debug/xdk/3rdparty/crypto/md5/md5.o

obj-debug/xdk/3rdparty/crypto/sha1/sha1.o: xdk/3rdparty/crypto/sha1/sha1.c xdk/3rdparty/crypto/sha1/sha1.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/3rdparty/crypto/sha1/sha1.c -o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o

obj-debug/xdk/3rdparty/libinotifytools/inotifytools.o: xdk/3rdparty/libinotifytools/inotifytools.c xdk/3rdparty/libinotifytools/inotifytools.h xdk/3rdparty/libinotifytools/inotifytools_p.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/3rdparty/libinotifytools/inotifytools.c -o obj-debug/xdk/3rdparty/libinotifytools/inotifytools.o

obj-debug/xdk/3rdparty/libinotifytools/redblack.o: xdk/3rdparty/libinotifytools/redblack.c xdk/3rdparty/libinotifytools/redblack.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/3rdparty/libinotifytools/redblack.c -o obj-debug/xdk/3rdparty/libinotifytools/redblack.o

obj-debug/xdk/xbigint.o: xdk/3rdparty/bigint/bigint.h xdk/xbigint.c xdk/xbigint.h xdk/xdef.h xdk/xmemory.h xdk/xstr.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xbigint.c -o obj-debug/xdk/xbigint.o

obj-debug/xdk/xconf.o: xdk/xconf.c xdk/xconf.h xdk/xdef.h xdk/xhash.h xdk/xmemory.h xdk/xstr.h xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xconf.c -o obj-debug/xdk/xconf.o

obj-debug/xdk/xcrypto.o: xdk/3rdparty/crypto/md5/md5.h xdk/3rdparty/crypto/sha1/sha1.h xdk/xcrypto.c xdk/xcrypto.h xdk/xdef.h xdk/xmemory.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xcrypto.c -o obj-debug/xdk/xcrypto.o

obj-debug/xdk/xdef.o: xdk/xdef.c xdk/xdef.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xdef.c -o obj-debug/xdk/xdef.o

obj-debug/xdk/xhash.o: xdk/xdef.h xdk/xhash.c xdk/xhash.h xdk/xmemory.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xhash.c -o obj-debug/xdk/xhash.o

obj-debug/xdk/xkeepalive.o: xdk/xdef.h xdk/xkeepalive.c xdk/xkeepalive.h xdk/xlog.h xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xkeepalive.c -o obj-debug/xdk/xkeepalive.o

obj-debug/xdk/xlog.o: xdk/xdef.h xdk/xlog.c xdk/xlog.h xdk/xoption.h xdk/xsys.h xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xlog.c -o obj-debug/xdk/xlog.o

obj-debug/xdk/xmemory.o: xdk/xdef.h xdk/xmemory.c xdk/xmemory.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xmemory.c -o obj-debug/xdk/xmemory.o

obj-debug/xdk/xnet.o: xdk/xdef.h xdk/xlog.h xdk/xmemory.h xdk/xnet.c xdk/xnet.h xdk/xstr.h xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xnet.c -o obj-debug/xdk/xnet.o

obj-debug/xdk/xoption.o: xdk/xconf.h xdk/xdef.h xdk/xhash.h xdk/xlog.h xdk/xmemory.h xdk/xoption.c xdk/xoption.h xdk/xstr.h xdk/xutils.h xdk/xvec.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xoption.c -o obj-debug/xdk/xoption.o

obj-debug/xdk/xstr.o: xdk/xdef.h xdk/xmemory.h xdk/xstr.c xdk/xstr.h xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xstr.c -o obj-debug/xdk/xstr.o

obj-debug/xdk/xsys.o: xdk/xsys.c xdk/xsys.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xsys.c -o obj-debug/xdk/xsys.o

obj-debug/xdk/xutils.o: xdk/xcrypto.h xdk/xdef.h xdk/xmemory.h xdk/xstr.h xdk/xsys.h xdk/xutils.c xdk/xutils.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xutils.c -o obj-debug/xdk/xutils.o

obj-debug/xdk/xvec.o: xdk/xmemory.h xdk/xvec.c xdk/xvec.h
	$(CC_debug) $(CFLAGS_debug) -c xdk/xvec.c -o obj-debug/xdk/xvec.o

xdk-debug: bin-debug obj-debug obj-debug/xdk/3rdparty/bigint/bigint.o obj-debug/xdk/3rdparty/crypto/md5/md5.o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o obj-debug/xdk/3rdparty/libinotifytools/inotifytools.o obj-debug/xdk/3rdparty/libinotifytools/redblack.o obj-debug/xdk/xbigint.o obj-debug/xdk/xconf.o obj-debug/xdk/xcrypto.o obj-debug/xdk/xdef.o obj-debug/xdk/xhash.o obj-debug/xdk/xkeepalive.o obj-debug/xdk/xlog.o obj-debug/xdk/xmemory.o obj-debug/xdk/xnet.o obj-debug/xdk/xoption.o obj-debug/xdk/xstr.o obj-debug/xdk/xsys.o obj-debug/xdk/xutils.o obj-debug/xdk/xvec.o


all-debug: bin-debug obj-debug obj-debug/3rdparty/vnc_auth/d3des.o obj-debug/src/vnc_proxy.o obj-debug/src/vnc_proxy_ctl.o obj-debug/xdk/3rdparty/bigint/bigint.o obj-debug/xdk/3rdparty/crypto/md5/md5.o obj-debug/xdk/3rdparty/crypto/sha1/sha1.o obj-debug/xdk/3rdparty/libinotifytools/inotifytools.o obj-debug/xdk/3rdparty/libinotifytools/redblack.o obj-debug/xdk/xbigint.o obj-debug/xdk/xconf.o obj-debug/xdk/xcrypto.o obj-debug/xdk/xdef.o obj-debug/xdk/xhash.o obj-debug/xdk/xkeepalive.o obj-debug/xdk/xlog.o obj-debug/xdk/xmemory.o obj-debug/xdk/xnet.o obj-debug/xdk/xoption.o obj-debug/xdk/xstr.o obj-debug/xdk/xsys.o obj-debug/xdk/xutils.o obj-debug/xdk/xvec.o 

3rdparty-release: bin-release obj-release obj-release/3rdparty/vnc_auth/d3des.o

obj-release/3rdparty/vnc_auth/d3des.o: 3rdparty/vnc_auth/d3des.c 3rdparty/vnc_auth/d3des.h
	$(CC_release) $(CFLAGS_release) -c 3rdparty/vnc_auth/d3des.c -o obj-release/3rdparty/vnc_auth/d3des.o


bin-release/vnc_proxy: obj-release/3rdparty/vnc_auth/d3des.o obj-release/src/vnc_proxy.o obj-release/xdk/3rdparty/crypto/md5/md5.o obj-release/xdk/3rdparty/crypto/sha1/sha1.o obj-release/xdk/xconf.o obj-release/xdk/xcrypto.o obj-release/xdk/xdef.o obj-release/xdk/xhash.o obj-release/xdk/xlog.o obj-release/xdk/xmemory.o obj-release/xdk/xnet.o obj-release/xdk/xoption.o obj-release/xdk/xstr.o obj-release/xdk/xsys.o obj-release/xdk/xutils.o obj-release/xdk/xvec.o
	$(CC_release) $(CFLAGS_release) $(LDFLAGS_release) obj-release/src/vnc_proxy.o obj-release/3rdparty/vnc_auth/d3des.o obj-release/xdk/3rdparty/crypto/md5/md5.o obj-release/xdk/3rdparty/crypto/sha1/sha1.o obj-release/xdk/xconf.o obj-release/xdk/xcrypto.o obj-release/xdk/xdef.o obj-release/xdk/xhash.o obj-release/xdk/xlog.o obj-release/xdk/xmemory.o obj-release/xdk/xnet.o obj-release/xdk/xoption.o obj-release/xdk/xstr.o obj-release/xdk/xsys.o obj-release/xdk/xutils.o obj-release/xdk/xvec.o -o bin-release/vnc_proxy

bin-release/vnc_proxy_ctl: obj-release/src/vnc_proxy_ctl.o obj-release/xdk/3rdparty/crypto/md5/md5.o obj-release/xdk/3rdparty/crypto/sha1/sha1.o obj-release/xdk/xcrypto.o obj-release/xdk/xdef.o obj-release/xdk/xmemory.o obj-release/xdk/xstr.o obj-release/xdk/xsys.o obj-release/xdk/xutils.o
	$(CC_release) $(CFLAGS_release) $(LDFLAGS_release) obj-release/src/vnc_proxy_ctl.o obj-release/xdk/3rdparty/crypto/md5/md5.o obj-release/xdk/3rdparty/crypto/sha1/sha1.o obj-release/xdk/xcrypto.o obj-release/xdk/xdef.o obj-release/xdk/xmemory.o obj-release/xdk/xstr.o obj-release/xdk/xsys.o obj-release/xdk/xutils.o -o bin-release/vnc_proxy_ctl

obj-release/src/vnc_proxy.o: 3rdparty/vnc_auth/d3des.h src/vnc_proxy.c xdk/xcrypto.h xdk/xlog.h xdk/xmemory.h xdk/xnet.h xdk/xstr.h xdk/xutils.h xdk/xvec.h
	$(CC_release) $(CFLAGS_release) -c src/vnc_proxy.c -o obj-release/src/vnc_proxy.o

obj-release/src/vnc_proxy_ctl.o: src/vnc_proxy_ctl.c xdk/xdef.h xdk/xmemory.h xdk/xstr.h xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c src/vnc_proxy_ctl.c -o obj-release/src/vnc_proxy_ctl.o

src-release: bin-release obj-release bin-release/vnc_proxy bin-release/vnc_proxy_ctl obj-release/src/vnc_proxy.o obj-release/src/vnc_proxy_ctl.o


runtest-release: test-release
	@clear
	@echo Running test module \'test\'
	@echo ==========================

test-release: bin-release obj-release 


obj-release/xdk/3rdparty/bigint/bigint.o: xdk/3rdparty/bigint/bigint.c xdk/3rdparty/bigint/bigint.h xdk/xbigint.h
	$(CC_release) $(CFLAGS_release) -c xdk/3rdparty/bigint/bigint.c -o obj-release/xdk/3rdparty/bigint/bigint.o

obj-release/xdk/3rdparty/crypto/md5/md5.o: xdk/3rdparty/crypto/md5/md5.c xdk/3rdparty/crypto/md5/md5.h
	$(CC_release) $(CFLAGS_release) -c xdk/3rdparty/crypto/md5/md5.c -o obj-release/xdk/3rdparty/crypto/md5/md5.o

obj-release/xdk/3rdparty/crypto/sha1/sha1.o: xdk/3rdparty/crypto/sha1/sha1.c xdk/3rdparty/crypto/sha1/sha1.h
	$(CC_release) $(CFLAGS_release) -c xdk/3rdparty/crypto/sha1/sha1.c -o obj-release/xdk/3rdparty/crypto/sha1/sha1.o

obj-release/xdk/3rdparty/libinotifytools/inotifytools.o: xdk/3rdparty/libinotifytools/inotifytools.c xdk/3rdparty/libinotifytools/inotifytools.h xdk/3rdparty/libinotifytools/inotifytools_p.h
	$(CC_release) $(CFLAGS_release) -c xdk/3rdparty/libinotifytools/inotifytools.c -o obj-release/xdk/3rdparty/libinotifytools/inotifytools.o

obj-release/xdk/3rdparty/libinotifytools/redblack.o: xdk/3rdparty/libinotifytools/redblack.c xdk/3rdparty/libinotifytools/redblack.h
	$(CC_release) $(CFLAGS_release) -c xdk/3rdparty/libinotifytools/redblack.c -o obj-release/xdk/3rdparty/libinotifytools/redblack.o

obj-release/xdk/xbigint.o: xdk/3rdparty/bigint/bigint.h xdk/xbigint.c xdk/xbigint.h xdk/xdef.h xdk/xmemory.h xdk/xstr.h
	$(CC_release) $(CFLAGS_release) -c xdk/xbigint.c -o obj-release/xdk/xbigint.o

obj-release/xdk/xconf.o: xdk/xconf.c xdk/xconf.h xdk/xdef.h xdk/xhash.h xdk/xmemory.h xdk/xstr.h xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c xdk/xconf.c -o obj-release/xdk/xconf.o

obj-release/xdk/xcrypto.o: xdk/3rdparty/crypto/md5/md5.h xdk/3rdparty/crypto/sha1/sha1.h xdk/xcrypto.c xdk/xcrypto.h xdk/xdef.h xdk/xmemory.h
	$(CC_release) $(CFLAGS_release) -c xdk/xcrypto.c -o obj-release/xdk/xcrypto.o

obj-release/xdk/xdef.o: xdk/xdef.c xdk/xdef.h
	$(CC_release) $(CFLAGS_release) -c xdk/xdef.c -o obj-release/xdk/xdef.o

obj-release/xdk/xhash.o: xdk/xdef.h xdk/xhash.c xdk/xhash.h xdk/xmemory.h
	$(CC_release) $(CFLAGS_release) -c xdk/xhash.c -o obj-release/xdk/xhash.o

obj-release/xdk/xkeepalive.o: xdk/xdef.h xdk/xkeepalive.c xdk/xkeepalive.h xdk/xlog.h xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c xdk/xkeepalive.c -o obj-release/xdk/xkeepalive.o

obj-release/xdk/xlog.o: xdk/xdef.h xdk/xlog.c xdk/xlog.h xdk/xoption.h xdk/xsys.h xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c xdk/xlog.c -o obj-release/xdk/xlog.o

obj-release/xdk/xmemory.o: xdk/xdef.h xdk/xmemory.c xdk/xmemory.h
	$(CC_release) $(CFLAGS_release) -c xdk/xmemory.c -o obj-release/xdk/xmemory.o

obj-release/xdk/xnet.o: xdk/xdef.h xdk/xlog.h xdk/xmemory.h xdk/xnet.c xdk/xnet.h xdk/xstr.h xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c xdk/xnet.c -o obj-release/xdk/xnet.o

obj-release/xdk/xoption.o: xdk/xconf.h xdk/xdef.h xdk/xhash.h xdk/xlog.h xdk/xmemory.h xdk/xoption.c xdk/xoption.h xdk/xstr.h xdk/xutils.h xdk/xvec.h
	$(CC_release) $(CFLAGS_release) -c xdk/xoption.c -o obj-release/xdk/xoption.o

obj-release/xdk/xstr.o: xdk/xdef.h xdk/xmemory.h xdk/xstr.c xdk/xstr.h xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c xdk/xstr.c -o obj-release/xdk/xstr.o

obj-release/xdk/xsys.o: xdk/xsys.c xdk/xsys.h
	$(CC_release) $(CFLAGS_release) -c xdk/xsys.c -o obj-release/xdk/xsys.o

obj-release/xdk/xutils.o: xdk/xcrypto.h xdk/xdef.h xdk/xmemory.h xdk/xstr.h xdk/xsys.h xdk/xutils.c xdk/xutils.h
	$(CC_release) $(CFLAGS_release) -c xdk/xutils.c -o obj-release/xdk/xutils.o

obj-release/xdk/xvec.o: xdk/xmemory.h xdk/xvec.c xdk/xvec.h
	$(CC_release) $(CFLAGS_release) -c xdk/xvec.c -o obj-release/xdk/xvec.o

xdk-release: bin-release obj-release obj-release/xdk/3rdparty/bigint/bigint.o obj-release/xdk/3rdparty/crypto/md5/md5.o obj-release/xdk/3rdparty/crypto/sha1/sha1.o obj-release/xdk/3rdparty/libinotifytools/inotifytools.o obj-release/xdk/3rdparty/libinotifytools/redblack.o obj-release/xdk/xbigint.o obj-release/xdk/xconf.o obj-release/xdk/xcrypto.o obj-release/xdk/xdef.o obj-release/xdk/xhash.o obj-release/xdk/xkeepalive.o obj-release/xdk/xlog.o obj-release/xdk/xmemory.o obj-release/xdk/xnet.o obj-release/xdk/xoption.o obj-release/xdk/xstr.o obj-release/xdk/xsys.o obj-release/xdk/xutils.o obj-release/xdk/xvec.o


all-release: bin-release obj-release obj-release/3rdparty/vnc_auth/d3des.o obj-release/src/vnc_proxy.o obj-release/src/vnc_proxy_ctl.o obj-release/xdk/3rdparty/bigint/bigint.o obj-release/xdk/3rdparty/crypto/md5/md5.o obj-release/xdk/3rdparty/crypto/sha1/sha1.o obj-release/xdk/3rdparty/libinotifytools/inotifytools.o obj-release/xdk/3rdparty/libinotifytools/redblack.o obj-release/xdk/xbigint.o obj-release/xdk/xconf.o obj-release/xdk/xcrypto.o obj-release/xdk/xdef.o obj-release/xdk/xhash.o obj-release/xdk/xkeepalive.o obj-release/xdk/xlog.o obj-release/xdk/xmemory.o obj-release/xdk/xnet.o obj-release/xdk/xoption.o obj-release/xdk/xstr.o obj-release/xdk/xsys.o obj-release/xdk/xutils.o obj-release/xdk/xvec.o 



runtest: runtest-debug



src: src-debug

xdk: xdk-debug

test: test-debug

3rdparty: 3rdparty-debug



