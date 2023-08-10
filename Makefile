all:
	gcc rwm.c -lX11 -lO3 -o rwm
	strip rwm

xinit:
	Xephyr -screen 1024x768 :2 &
	sleep 1
	xterm  -display :2 &

debug:
	gcc rwm.c -lX11 -o rwm
	./rwm