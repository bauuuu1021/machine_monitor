#include "client.h"

void request ();

int main(int argc, char **argv)
{
	struct sockaddr_in server;
	int sock;
	char buf[100];

	// create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);

	// set socket
	server.sin_family = AF_INET;
	server.sin_port = htons(59487); //connect with port 59487

	// ip = 127.0.0.1 means connect to localhost
	inet_pton(AF_INET, "127.0.0.1", &server.sin_addr.s_addr);

	//connect with server
	connect(sock, (struct sockaddr *)&server, sizeof(server));

	//try to send message to server
	char cmd[4],pid[8];
	int i;
	char receive[5];
	char receivefh[4];

	//connect with server until exit is request
	while (1) {
		request();  //call function to print out message

		//input cmd
		printf("which?\t");
		bzero(cmd,4);
		fgets(cmd,3,stdin);
		write(sock,cmd,strlen(cmd));

		if (cmd[0]=='k') {  //exit
			close(sock);
			return 0;
		} else if (cmd[0]=='c') { //child id
			//input pid
			printf("pid?\t");
			bzero(pid,8);
			fgets(pid,7,stdin);
			write(sock,pid,strlen(pid));

			printf("receive:\n");
			while(1) {
				//receive response from server
				memset(buf, 0, sizeof(buf));
				read(sock, buf, sizeof(buf));
				if (atoi(buf)==-1) {
					printf("end\n");
					break;
				}
				printf("%s\n",buf);
			}

		} else if (cmd[0]=='h'||cmd[0]=='b') {
			//input pid
			printf("pid?\t");
			bzero(pid,8);
			fgets(pid,7,stdin);
			write(sock,pid,strlen(pid));

			char numAnswerC[2];
			memset(numAnswerC,0,sizeof(numAnswerC));
			read(sock,numAnswerC,sizeof(numAnswerC));
			int numAnswer = atoi(numAnswerC);

			//receive array
			printf("receive:\n");
			for (i=0; i<numAnswer; i++) {
				memset(receivefh,0,sizeof(receivefh));
				read(sock,receivefh,sizeof(receivefh));
				printf("%s\n",receivefh);
			}
			printf("end\n");

		} else if (cmd[0]!='a') {
			//input pid
			printf("pid?\t");
			bzero(pid,8);
			fgets(pid,7,stdin);
			write(sock,pid,strlen(pid));

			//receive response from server
			memset(buf, 0, sizeof(buf));
			read(sock, buf, sizeof(buf));
			printf("receive: %s\n",buf);

		} else if (cmd[0]=='a') {
			//receive num of process first

			char numProcessC[4];
			memset(numProcessC,0,sizeof(numProcessC));
			read(sock,numProcessC,sizeof(numProcessC));
			int numProcess = atoi(numProcessC);

			//receive array

			for (i=0; i<numProcess+1; i++) {
				memset(receive,0,sizeof(receive));
				read(sock,receive,sizeof(receive));
				printf("%s\n",receive);
			}

		}
	}   //end while

	//close(sock);
	//return 0;
}

void request()
{
	printf("============================================\n");
	printf("(a)list all process ids\n");
	printf("(b)thread's IDs\n");
	printf("(c)child's PIDs\n");
	printf("(d)process name\n");
	printf("(e)state of process(D,R,S,T,t,W,X,Z)\n");
	printf("(f)command line of excuting process(cmdline)\n");
	printf("(g)parent's PID\n");
	printf("(h)all ancients of PIDs\n");
	printf("(i)virtual memory size(VmSize)\n");
	printf("(j)physical memory size(VmRSS)\n");
	printf("(k)exit\n");
}
