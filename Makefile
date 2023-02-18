.PHONY: clean

asdcontrol: asdcontrol.cpp FORCE
	g++ -Og asdcontrol.cpp -o asdcontrol

debug: asdcontrol.cpp FORCE
	g++ -Og -g asdcontrol.cpp -o asdcontrol

clean:
	rm -f asdcontrols

FORCE:
