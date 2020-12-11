default:
	make server
	make client
server:
	gcc -Wall dfs.c -o dfs
client:
	gcc -Wall dfc.c -I /usr/local/opt/openssl/include -L /usr/local/opt/openssl/lib -lcrypto -o dfc
