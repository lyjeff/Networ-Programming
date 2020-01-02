all:
	g++ -o hw3 hw3.cpp -lpcap


file:
	sudo ./hw3 -r 1.pcap > output/out_1
	sudo ./hw3 -r input/arp-storm.pcap > output/out_arp-storm
	sudo ./hw3 -r input/dns.cap > output/out_dns
	sudo ./hw3 -r input/http.cap > output/out_http
	sudo ./hw3 -r input/ospf.cap > output/out_ospf
	sudo ./hw3 -r input/v6-http.cap > output/out_v6-http

real:
	sudo ./hw3

clean:
	rm -f hw3 output/*