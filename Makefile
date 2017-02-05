INCLUDE_DIR=$(shell echo ~)/local/include
LIBRARY_DIR=$(shell echo ~)/local/lib
DESTDIR=/
PREFIX=/usr/

CXX=g++
CXXFLAGS=-L${LIBRARY_DIR} -I${INCLUDE_DIR} -O2 -g -std=c++11 -fPIC -Wall -Wextra

all: wd test wdclient libraries headers wdctl

install: wd wdclient wdctl libraries headers
	mkdir -p ${DESTDIR}/${PREFIX}/lib
	mkdir -p ${DESTDIR}/${PREFIX}/bin
	mkdir -p ${DESTDIR}/${PREFIX}/include/watchdog
	mkdir -p ${DESTDIR}/etc/watchdog/
	cp *.a ${DESTDIR}/${PREFIX}/lib
	cp *.so ${DESTDIR}/${PREFIX}/lib
	cp src/client.h ${DESTDIR}/${PREFIX}/include/watchdog/
	cp wd ${DESTDIR}/${PREFIX}/bin
	cp wdclient ${DESTDIR}/${PREFIX}/bin
	cp wdctl ${DESTDIR}/${PREFIX}/bin
	cp wd.conf ${DESTDIR}/etc/watchdog/

uninstall:
	rm -f ${DESTDIR}/${PREFIX}/bin/wdclient
	rm -f ${DESTDIR}/${PREFIX}/bin/wdctl
	rm -f ${DESTDIR}/${PREFIX}/bin/wd
	rm -f ${DESTDIR}/${PREFIX}/lib/libwatchdog.so
	rm -f ${DESTDIR}/${PREFIX}/lib/libwatchdog.a
	rm -rf ${DESTDIR}/etc/watchdog/wd.conf
	rm -rf ${DESTDIR}/${PREFIX}/include/watchdog/

wd: src/wd.cc src/server_config.h watchdog.pb.o task.o server_config.o common_config.o encode.o
	${CXX} ${CXXFLAGS} src/wd.cc watchdog.pb.o task.o server_config.o common_config.o encode.o -o wd -lprotobuf -lpthread -lstdc++ -lboost_program_options -lsmplsocket -ltxtable -lslog

wdctl: src/wdctl.cc watchdog.pb.o common_config.o
	${CXX} ${CXXFLAGS} src/wdctl.cc watchdog.pb.o common_config.o -o wdctl -lprotobuf -lpthread -lstdc++ -lsmplsocket -lboost_program_options -ltxtable -lslog

test: src/test.cc client.o watchdog.pb.o common_config.o client_config.o encode.o
	${CXX} ${CXXFLAGS} src/test.cc client.o watchdog.pb.o common_config.o client_config.o encode.o -o test -lprotobuf -lpthread -lstdc++ -lsmplsocket -lboost_program_options -lslog

wdclient: src/wdclient.cc client.o watchdog.pb.o common_config.o client_config.o encode.o
	${CXX} ${CXXFLAGS} src/wdclient.cc client.o watchdog.pb.o common_config.o client_config.o encode.o -o wdclient -lprotobuf -lpthread -lstdc++ -lsmplsocket -lboost_program_options -lslog

headers: src/client.h

libraries: libwatchdog.so libwatchdog.a

libwatchdog.so: client.o watchdog.pb.o
	${CXX} ${CXXFLAGS} -shared -Wl,-soname,libwatchdog.so -o libwatchdog.so client.o watchdog.pb.o

libwatchdog.a: client.o watchdog.pb.o
	ar rcs libwatchdog.a client.o watchdog.pb.o

client.o: src/client.cc src/client.h
	${CXX} ${CXXFLAGS} -c src/client.cc -o client.o

encode.o: src/encode.cc src/encode.h
	${CXX} ${CXXFLAGS} -c src/encode.cc -o encode.o

task.o: src/task.cc src/task.h
	${CXX} ${CXXFLAGS} -c src/task.cc -o task.o

server_config.o: src/server_config.cc src/server_config.h
	${CXX} ${CXXFLAGS} -c src/server_config.cc -o server_config.o

client_config.o: src/client_config.cc src/client_config.h
	${CXX} ${CXXFLAGS} -c src/client_config.cc -o client_config.o

common_config.o: src/common_config.cc src/common_config.h
	${CXX} ${CXXFLAGS} -c src/common_config.cc -o common_config.o

watchdog.pb.o: watchdog.proto
	mkdir -p src/
	protoc --cpp_out='src/' watchdog.proto
	${CXX} ${CXXFLAGS} -c src/watchdog.pb.cc -o watchdog.pb.o

clean:
	rm -f src/watchdog.pb.cc
	rm -f src/watchdog.pb.h
	rm -f wd
	rm -f test
	rm -f wdclient
	rm -f wdctl
	rm -f *.o
	rm -f libwatchdog.a
	rm -f libwatchdog.so
