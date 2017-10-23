all:	check

test:	clean-ckpt libckpt.so hello 
	LD_PRELOAD=`pwd`/libckpt.so ./hello

default: check
		
clean-ckpt:
	rm -rf myckpt

clean: clean-ckpt
	rm -rf restart hello hello.o ckpt ckpt.o libckpt.so

ckpt.o: ckpt.c
	gcc -c -Wall -Werror -fpic -o ckpt.o -fno-stack-protector ckpt.c

libckpt.so: ckpt.o
	gcc -shared -o libckpt.so ckpt.o

hello.o: hello.c
	gcc -c -Wall -Werror -fpic -o hello.o -fno-stack-protector hello.c

hello:	hello.o
	gcc -g -o hello hello.o

restart: restart.c
	gcc -g -static -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 -o restart -fno-stack-protector restart.c

res: 	restart
	(sleep 7 && pkill -9 restart) &
	./restart myckpt

gdb:
	gdb --args ./restart myckpt

check:	clean libckpt.so hello restart
	(sleep 3 && kill -12 `pgrep -n hello` && sleep 2 && pkill -9 hello && make res) & 
	LD_PRELOAD=`pwd`/libckpt.so ./hello

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
