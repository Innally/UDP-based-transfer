.PHONY: s st c ct server client

server:s st

client:c ct

s:
	g++ main.cpp -o smain -lpthread

st:
	./smain

c:
	g++ client.cpp -o cmain

ct:
	./cmain