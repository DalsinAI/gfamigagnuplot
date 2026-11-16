# PC tests: make test (host-gnuplot.sh fetches gnuplot's tarball at gnuplot.lock's checksum)
CC ?= cc
test:
	sh test-part.sh
	mkdir -p ../build-host
	$(CC) -Wall -Wextra -I../amiga -o ../build-host/core_test core_test.c ../amiga/gfos_core.c && ../build-host/core_test
	sh host-gnuplot.sh
.PHONY: test
