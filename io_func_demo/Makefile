
target:01-server 01-client 02-server 02-client 03-server 03-client
	echo "finished!!"

01-server:
	gcc  server.c recv_send_common.c -o 01-server
02-server:
	gcc  server.c readv_writev_common.c -o 02-server
03-server:
	gcc  server.c recvmsg_sendmsg_common.c -o 03-server

01-client:
	gcc client.c recv_send_common.c -o 01-client
02-client:
	gcc client.c readv_writev_common.c -o 02-client
03-client:
	gcc client.c readv_writev_common.c -o 03-client

clean:
	rm -rf 01-server 01-client 02-server 02-client 03-server 03-client
