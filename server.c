#include "server.h"
#define pidRange 10000

void *assign(void *sockId);

int main(int argc, char **argv)
{
	int sock0;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	int sock_client;

	// create socket
	sock0 = socket(AF_INET, SOCK_STREAM, 0);

	// set socket
	addr.sin_family = AF_INET;
	addr.sin_port = htons(59487);  //connect with port 59487
	addr.sin_addr.s_addr = INADDR_ANY;
	bind(sock0, (struct sockaddr*)&addr, sizeof(addr));

	// wait for TCP clients' request
	listen(sock0, 50);

	// accept connection request from TCP client
	socklen_t len = sizeof(client);

	while (sock_client = accept(sock0, (struct sockaddr *)&client, &len)) {
		pthread_t thread;
		int *sockId = malloc(1);
		*sockId = sock_client;
		pthread_create(&thread,NULL,assign,(void*)sockId);

	}
}

void *assign(void* sockId)
{
	int sock_client =*(int*)sockId;
	printf("connect successed\n");

	//receive cmd and pid, then find answer and respond
	char cmd[4],pid[8];
	int tgid,tgidfa/*tgid for a*/;   //receive pid from client and put it inside
	int numProcess=0,numAnswer=0;   //count how much answer are there
	int current;    //used when searching target
	int i;  //for loop
	int listAll[500]= {0},listAnswer[500]= {0};  //list all process or answer
	char path[40], temp[100], *p, sent[5];
	FILE * fp;
	int checkIJ=0;  //check if cmd i,j exist

	while (1) {
		//get cmd from client
		memset(cmd,0,sizeof(cmd));
		bzero(cmd,4);
		read(sock_client,cmd,3);

		//except cmd a and k, others need to read in pid
		if (cmd[0]!='a' && cmd[0]!='k') {
			memset(pid,0,sizeof(pid));
			bzero(pid,8);
			read(sock_client,pid,7);
			tgid=atoi(pid);
		}

		//do something depend on cmd
		switch (cmd[0]) {
		case 'a':   //list all process ids
			tgidfa=1;
			numProcess=0;

			while (tgidfa<pidRange) {
				snprintf(path,40,"/proc/%d/status",tgidfa);
				fp=fopen(path,"r");

				if (fp) {
					listAll[numProcess]=tgidfa;
					numProcess++;
				}
				tgidfa++;
				close(fp);
			}

			snprintf(sent,sizeof(sent),"%d",numProcess);    //sent num of process first
			write(sock_client,sent,sizeof(sent));

			for (i=0; i<numProcess; i++) {
				snprintf(sent,sizeof(sent),"%d",listAll[i]);
				write(sock_client,sent,sizeof(
				          sent));   //then send listAll[] one by one
			}

			break;
		case 'b':  ; //thread's ID

			snprintf(path, 40, "/proc/%d/task", tgid);
			DIR * d;
			d= opendir(path);
			struct dirent * dir;
			char *tempfb;
			numAnswer=0;


			while ((dir=readdir(d))!=NULL) {
				//check if the dir is . or ..
				if ((dir->d_name[0]=='.')&&(!dir->d_name[1]||(dir->d_name[1]=='.'
				                            &&!dir->d_name[2])))
					continue;


				listAnswer[numAnswer]=atoi(dir->d_name);

				numAnswer++;
			}

			//sent num of answer first
			snprintf(sent,sizeof(sent),"%d",numAnswer);
			write(sock_client,sent,2);

			//then sent the answerList
			for (i=0; i<numAnswer; i++) {
				memset(sent,0,sizeof(sent));
				snprintf(sent,sizeof(sent),"%.4d",listAnswer[i]);
				write(sock_client,sent,strlen(sent));   //then send listAll[    ] one by one
			}

			closedir(d);
			break;
		case 'c':   //child's PIDs
			//get numProcess and list all pid first
			tgidfa=1;
			numProcess=0;
			numAnswer=0;

			while (tgidfa<pidRange) {
				snprintf(path,40,"/proc/%d/status",tgidfa);
				fp=fopen(path,"r");

				if (fp) {
					listAll[numProcess]=tgidfa;
					numProcess++;
				}
				tgidfa++;
				close(fp);
			}

			//then check every pid if the ppid of current pid == input pid
			for (i=0; i<numProcess; i++) {
				snprintf(path, 40, "/proc/%d/status", listAll[i]);
				fp = fopen(path, "r");

				if(fp) {

					while(fgets(temp, 100, fp)) {
						if(strncmp(temp, "PPid:", 5) != 0)
							continue;

						// Ignore "PPid:" and whitespace
						p = temp + 6;

						while(isspace(*p))
							++p;

						//sent answer back to server
						if (atoi(p)==tgid) {    //check if (current ppid)==(input pid)
							snprintf(sent,sizeof(sent),"%d",listAll[i]);
							write(sock_client,sent,5);
						}
					}
				}
			}

			//after sent all answers, sent "-1" as end label
			snprintf(sent,sizeof(sent),"-1");
			write(sock_client,sent,5);

			break;
		case 'd':   //process name
			snprintf(path, 40, "/proc/%d/status", tgid);
			fp = fopen(path, "r");

			if(!fp) {
				write(sock_client,"Invalid PID",20);
				break;
			}


			while(fgets(temp, 100, fp)) {
				if(strncmp(temp, "Name:", 5) != 0) //check if current line is requested info.
					continue;

				// Ignore "Name:" and whitespace
				p = temp + 6;

				while(isspace(*p))  //make p pointer directly to non-space char
					++p;

				//sent answer back to client
				write(sock_client,(void*)p,20);
				break;
			}
			break;

		case 'e':   //state of process(D,R,S,T,t,W,X,Z)
			snprintf(path, 40, "/proc/%d/status", tgid);
			fp = fopen(path, "r");

			if(!fp) {
				write(sock_client,"Invalid PID",20);
				break;
			}


			while(fgets(temp, 100, fp)) {
				if(strncmp(temp, "State:", 6) != 0) //check if current line is requested info.
					continue;

				// Ignore "State:" and whitespace
				p = temp + 7;

				while(isspace(*p)) //make p pointer directly to non-space char
					++p;

				//sent answer back to client
				write(sock_client,(void*)p,20);
				break;
			}

			break;

		case 'f':   //command line of excuting process(cmdline)
			snprintf(path,40,"/proc/%d/cmdline", tgid);
			fp=fopen(path,"r");

			if(!fp) {
				write(sock_client,"Invalid PID",20);
				break;
			}


			if(fgets(temp,100,fp))
				write(sock_client,(void*)temp,100);



			else


				write(sock_client,"Cmdline is empty",20);




			break;

		case 'g':   //parent's PID
			snprintf(path, 40, "/proc/%d/status", tgid);
			fp = fopen(path, "r");

			if(!fp) {
				write(sock_client,"Invalid PID",20);
				break;
			}



			while(fgets(temp, 100, fp)) {
				if(strncmp(temp, "PPid:", 5) != 0) ///////////////
					continue;

				// Ignore "PPid:" and whitespace
				p = temp + 6;   //////////////////////////////

				while(isspace(*p))
					++p;

				//put answer into response[]
				write(sock_client,(void*)p,20);
				break;
			}

			break;
		case 'h': ;  //all ancients of Pid

			numAnswer=0;

			while(1) {  //find ppid, ppid of ppid, and... until find pid=0
				snprintf(path, 40, "/proc/%d/status", tgid);
				fp = fopen(path, "r");

				if(!fp)
					return;

				while(fgets(temp, 100, fp)) {
					if(strncmp(temp, "PPid:", 5) != 0)
						continue;

					// Ignore "PPid:" and whitespace
					p = temp + 6;

					while(isspace(*p))
						++p;

					tgid=atoi(p);   //set new current pid = ppid of old current
					listAnswer[numAnswer]=tgid; //and record it

					break;
				}

				if (tgid==0)    //if pid=0 is the first process so can break the while loop
					break;
				numAnswer++;    //else keep on tracing
			}

			//sent num of answer first
			snprintf(sent,sizeof(sent),"%d",numAnswer);
			write(sock_client,sent,2);

			//then sent the answerList
			for (i=0; i<numAnswer; i++) {
				memset(sent,0,sizeof(sent));
				snprintf(sent,sizeof(sent),"%.4d",listAnswer[i]);
				write(sock_client,sent,strlen(sent));   //then send listAll[] one by one
			}

			break;

		case 'i':   //virtual memory size(VmSize)
			snprintf(path, 40, "/proc/%d/status", tgid);
			fp = fopen(path, "r");
			checkIJ=0;

			if(!fp) {
				write(sock_client,"Invalid PID",20);
				break;
			}


			while(fgets(temp, 100, fp)) {
				if(strncmp(temp, "VmSize:", 7) != 0) ///////////////
					continue;

				checkIJ=1;  //find answer
				// Ignore "VmSize:" and whitespace
				p = temp + 8;   //////////////////////////////

				while(isspace(*p))
					++p;

				//put answer into response[]
				write(sock_client,(void*)p,20);

				break;
			}
			if (!checkIJ)   //answer not found
				write(sock_client,"Not found in status",30);


			break;
		case 'j':   //physical memory size(VmRSS)
			snprintf(path, 40, "/proc/%d/status", tgid);
			fp = fopen(path, "r");
			checkIJ=0;

			if(!fp) {
				write(sock_client,"Invalid PID",20);
				break;
			}


			while(fgets(temp, 100, fp)) {
				if(strncmp(temp, "VmRSS:", 6) != 0) ///////////////
					continue;

				checkIJ=1;  //find answer
				// Ignore "VmRSS:" and whitespace
				p = temp + 7;   //////////////////////////////

				while(isspace(*p))
					++p;

				//put answer into response[]
				write(sock_client,(void*)p,20);
				break;
			}

			if (!checkIJ)   //answer not found
				write(sock_client,"Not found in status",30);

			break;
		default:

			printf("client break connection\n");

			//close
			close(sock_client);

			return 0;

		}   //end switch

	}   //end while

}   //end program
