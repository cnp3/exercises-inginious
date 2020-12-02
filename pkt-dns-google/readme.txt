This trace was created using

www.google.com

dig +bufsize=0 +noedns -t NS com @2001:7fd::1
dig +bufsize=0 +noedns -t NS google.com @2001:503:a83e::2:30
dig +bufsize=0 +noedns -t AAAA  www.google.com @2001:4860:4802:34::a


Conversion:

tshark -n -J "dns" -T pdml -r dns-example.pcap  -o udp.summary_in_tree:FALSE
