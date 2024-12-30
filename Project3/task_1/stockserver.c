/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct item{
    int id;
    int left_stock;
    int price;
    int readcnt; // 
    sem_t mutex;
    struct item *left;
    struct item *right;
} item;

typedef struct pool{
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];
} pool;

char ans[MAXBUF];
item* root = NULL;

void echo(int connfd);
void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);
item *createNode(int id,int left_stock,int price);
item *insertNode(item* root, int id,int left_stock,int price);
item *searchTree(item* root, int id);
void printTree(item* root);
void stockinit(void);
void getstockinfo(FILE *inputfile, item* root);
void update_stock(FILE *inputfile, item* root);
void sigint_handler(int signal);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static pool pool;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    stockinit();
    Signal(SIGINT,sigint_handler);
    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd,&pool);

    while (1) {
	    /* Wait for listening/connected descriptor(s) to become ready */
	    pool.ready_set = pool.read_set;
	    pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

	    /* If listening descriptor ready, add new client to pool */
	    if (FD_ISSET(listenfd, &pool.ready_set)) { //line:conc:echoservers:listenfdready
            clientlen = sizeof(struct sockaddr_storage);
	        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:conc:echoservers:accept
    
            Getnameinfo((SA *)&clientaddr,clientlen,client_hostname,MAXLINE,client_port,MAXLINE,0);
            printf("Connected to (%s, %s)\n", client_hostname, client_port);
            add_client(connfd, &pool); //line:conc:echoservers:addclient
	    }
    
	    /* Echo a text line from each ready connected descriptor */ 
	    check_clients(&pool); //line:conc:echoservers:checkclients
    }

    exit(0);
}
/* $end echoserverimain */
void sigint_handler(int signal){
    
    int olderrno = errno;

    FILE *fp = fopen("stock.txt", "w");
    
    if (!fp) {
        fprintf(stderr, "'stock.txt' file open error!\n");
        exit(1);
    }
    
    update_stock(fp, root);
    Sio_puts("Update complete\n");
    fclose(fp);
    exit(0);

    errno = olderrno;
}
/* $begin init_pool */
void init_pool(int listenfd, pool *p) 
{
    /* Initially, there are no connected descriptors */
    int i;
    p->maxi = -1;                   //line:conc:echoservers:beginempty
    for (i=0; i< FD_SETSIZE; i++)  
	p->clientfd[i] = -1;        //line:conc:echoservers:endempty

    /* Initially, listenfd is only member of select read set */
    p->maxfd = listenfd;            //line:conc:echoservers:begininit
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set); //line:conc:echoservers:endinit
}
/* $end init_pool */

/* $begin add_client */
void add_client(int connfd, pool *p) 
{
    int i=0;
    p->nready--;
    for (; i < FD_SETSIZE; i++)  /* Find an available slot */
	if (p->clientfd[i] < 0) { 
	    /* Add connected descriptor to the pool */
	    p->clientfd[i] = connfd;                 //line:conc:echoservers:beginaddclient
	    Rio_readinitb(&p->clientrio[i], connfd); //line:conc:echoservers:endaddclient

	    /* Add the descriptor to descriptor set */
	    FD_SET(connfd, &p->read_set); //line:conc:echoservers:addconnfd

	    /* Update max descriptor and pool highwater mark */
	    if (connfd > p->maxfd) //line:conc:echoservers:beginmaxfd
		    p->maxfd = connfd; //line:conc:echoservers:endmaxfd
	    if (i > p->maxi)       //line:conc:echoservers:beginmaxi
		    p->maxi = i;       //line:conc:echoservers:endmaxi
	    // byte_cnt++;
        break;
	}
    if (i == FD_SETSIZE) /* Couldn't find an empty slot */
	app_error("add_client error: Too many clients");
}
/* $end add_client */

/* $begin check_clients */
void check_clients(pool *p) // modify this
{
    int i, connfd, n;
    int s_id, s_cnt;
    char cmd[8];
    char response[30] = {0};
    char buf[MAXLINE]; 
    rio_t rio;

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++) {
	    connfd = p->clientfd[i];
	    rio = p->clientrio[i];
    
	    /* If the descriptor is ready, echo a text line from it */
	    if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) { 
	        p->nready--;
	        if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                sscanf(buf,"%s %d %d", cmd, &s_id, &s_cnt);
	    	    // byte_cnt += n; //line:conc:echoservers:beginecho
	    	    printf("Server received %d  bytes on fd %d\n",n, connfd);
                printf("cmd : %s s_id : %d s_cnt : %d\n",cmd, s_id,s_cnt);
	    	    // Rio_writen(connfd, buf, n); //line:conc:echoservers:endecho
                if(!strcmp(cmd,"show")){
                    printTree(root);
                    Rio_writen(connfd,ans,MAXBUF);
                    memset(ans,0,MAXBUF);
                }
                else if(!strncmp(cmd,"buy",3)){
                    struct item* ptr;
                    ptr = searchTree(root,s_id);
                    if(ptr->left_stock >= s_cnt){
                        ptr->left_stock -= s_cnt;
                        // fprintf(stdout,"buying\n");
                        snprintf(response,sizeof(response),"[buy] success\n");
                        Rio_writen(connfd,response,MAXBUF);
                        memset(response,0,30);
                    }
                    else{
                        // fprintf(stdout,"cannot buy\n");
                        snprintf(response,sizeof(response),"Not enough left stocks\n");
                        Rio_writen(connfd,response,MAXBUF);
                        memset(response,0,30);
                    }
                }
                else if(!strncmp(cmd,"sell",4)){
                    struct item* ptr;
                    ptr = searchTree(root,s_id);
                    ptr->left_stock += s_cnt;
                    snprintf(response,sizeof(response),"[sell] success\n");
                    Rio_writen(connfd,response,MAXBUF);
                    memset(response,0,30);
                }
                // else if(!strncmp(cmd,"exit",4)){
                //     snprintf(response,sizeof(response),"disconnection with server\n");
                //     Rio_writen(connfd,response,MAXBUF);
                //     memset(response,0,30);
                //     Close(connfd); //line:conc:echoservers:closeconnfd
	    	    //     FD_CLR(connfd, &p->read_set); //line:conc:echoservers:beginremove
	    	    //     p->clientfd[i] = -1;
                //     update_stock(fp,root);
                //     memset(ans,0,MAXBUF);
                //     // fclose(fp);
                // }
                memset(ans,0,MAXBUF);
            }
    
	        /* EOF detected, remove descriptor from pool */
	        else { 
	    	    Close(connfd); //line:conc:echoservers:closeconnfd
	    	    FD_CLR(connfd, &p->read_set); //line:conc:echoservers:beginremove
	    	    p->clientfd[i] = -1;
                memset(ans,0,MAXBUF);
            }
	    }
    }
}
/* $end check_clients */
item *createNode(int id,int left_stock,int price){
    item* node = (item*)malloc(sizeof(item));
    node->id = id;
    node->left_stock = left_stock;
    node->price = price;
    node->readcnt = 0;
    node->left = node->right = NULL;
    return node;
}
item *insertNode(item* root, int id,int left_stock,int price){
    if(root == NULL) return createNode(id,left_stock,price);
    
    if(id < root->id) root->left = insertNode(root->left,id,left_stock,price);
    else if(id > root->id) root->right = insertNode(root->right,id,left_stock,price);
    // else -> error문 

    return root;
}
item *searchTree(item* root, int id){
    if(root == NULL || root->id == id) return root;

    if(id < root->id) return searchTree(root->left,id);
    else return searchTree(root->right,id);
}
void update_stock(FILE *inputfile, item* root){ 

    if(root == NULL) return;

    update_stock(inputfile,root->left);
    fprintf(inputfile,"%d %d %d\n",root->id,root->left_stock,root->price);
    update_stock(inputfile,root->right);
}
void printTree(item* root){
    char temp[MAXLINE];
    int n;
    if(root == NULL) return;

    printTree(root->left);
    n =sprintf(temp,"%d %d %d\n",root->id,root->left_stock,root->price);
    strncat(ans,temp,n);
    printTree(root->right);

}
void stockinit(void){
    int id,left_stock,price; 
    // char buf[MAXLINE];
    FILE *fp = fopen("stock.txt","r");

    if(!fp){
        fprintf(stderr,"'stock.txt' open(read) error!\n");
        exit(1);
    }

    while(fscanf(fp,"%d %d %d",&id, &left_stock,&price) != EOF){
        root = insertNode(root,id,left_stock,price);
        // fprintf(stdout,"%d %d %d",id, left_stock,price);
    }

    fclose(fp);
    fp = NULL;
}


