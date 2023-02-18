.PHONY: clean

asdcontrol: asdcontrol.cpp FORCE
	g++ asdcontrol.cpp -o asdcontrol

clean:
	rm -f asdcontrols

FORCE:
