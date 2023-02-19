.PHONY: clean

asdcontrol: asdcontrol.cpp
	g++ -Og asdcontrol.cpp -o asdcontrol

debug: asdcontrol.cpp FORCE
	g++ -Og -g asdcontrol.cpp -o asdcontrol

clean:
	rm -f asdcontrols

install: asdcontrol
	cp asdcontrol /usr/local/bin/asdcontrol

FORCE:
