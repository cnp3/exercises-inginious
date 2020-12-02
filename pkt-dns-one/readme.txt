Context

Request for 

; <<>> DiG 9.10.6 <<>> -t AAAA one.one.one.one
;; global options: +cmd
;; Got answer:
;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 9607
;; flags: qr rd ra; QUERY: 1, ANSWER: 2, AUTHORITY: 2, ADDITIONAL: 4

;; OPT PSEUDOSECTION:
; EDNS: version: 0, flags:; udp: 4096
;; QUESTION SECTION:
;one.one.one.one.		IN	AAAA

;; ANSWER SECTION:
one.one.one.one.	300	IN	AAAA	2606:4700:4700::1111
one.one.one.one.	300	IN	AAAA	2606:4700:4700::1001

;; AUTHORITY SECTION:
one.one.one.	3599	IN	NS	fred.ns.cloudflare.com.
one.one.one.		3599	IN	NS	jean.ns.cloudflare.com.

;; ADDITIONAL SECTION:
fred.ns.cloudflare.com.	83350	IN	AAAA	2400:cb00:2049:1::adf5:3b71
fred.ns.cloudflare.com.	83350	IN	A	173.245.59.113
jean.ns.cloudflare.com.	11061	IN	A	173.245.58.121

;; Query time: 1016 msec
;; SERVER: 2001:6a8:3081:1::53#53(2001:6a8:3081:1::53)
;; WHEN: Fri Oct 11 09:50:13 CEST 2019
;; MSG SIZE  rcvd: 215

