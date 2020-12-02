This trace was created using

dig www.example.com
 dig +bufsize=0 +noedns -t NS com @2001:7fd::1
 dig +bufsize=0 +noedns -t NS example.com @2001:501:b1f9::30
 dig +bufsize=0 +noedns -t NS net @2001:7fd::1
 dig +bufsize=0 +noedns -t NS iana-servers.net @2001:501:b1f9::30
 dig +bufsize=0 +noedns -t AAAA www.example.com @2001:500:8f::53


Conversion:

tshark -n -J "dns" -T pdml -r dns-example.pcap  -o udp.summary_in_tree:FALSE
