include config.mk

rtree: rtree.o

watch:
	while ! inotifywait -e modify *.cc *.h; do make; done

clean:
	$(RM) *.o rtree

.PHONY: watch clean
