#!/bin/bash
tshark -T pdml -r file.pcap -J "tcp" -r $1 > $1.xml