all:
	g++ -std=c++11 -O2 receiver.cpp -o receiver_sigalrm
	g++ -std=c++11 -O2 sender_sigalrm.cpp -o sender_sigalrm
	g++ -std=c++11 -O2 receiver.cpp -o receiver_select
	g++ -std=c++11 -O2 sender_select.cpp -o sender_select
	g++ -std=c++11 -O2 receiver.cpp -o receiver_sockopt
	g++ -std=c++11 -O2 sender_sockopt.cpp -o sender_sockopt
test: all

clean:
	rm -rf receiver_sigalrm
	rm -rf sender_sigalrm
	rm -rf receiver_select
	rm -rf sender_select
	rm -rf receiver_sockopt
	rm -rf sender_sockopt

	

