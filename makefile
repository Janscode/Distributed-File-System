default:
	gcc -Wall dfc.c -I /usr/local/opt/openssl/include -L /usr/local/opt/openssl/lib -lcrypto
server:
	gcc -Wall dfs.c
