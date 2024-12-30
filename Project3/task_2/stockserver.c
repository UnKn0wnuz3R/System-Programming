/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

#define SBUFSIZE 1024
#define NTHREADS 256

typedef struct item{
    int id;
    int left_stock;
    int price;
    int readcnt; // 
    sem_t mutex;
    sem_t write_mutex;
    struct item *left;
    struct item *right;
} item;

typedef struct sbuf{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

sbuf_t sbuf;
static int byte_cnt;
// static int client_num=0;
static sem_t mutex;
char ans[MAXBUF];
item* root = NULL;

void echo_cnt(int connfd);
void sbuf_init(sbuf_t *sp,int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

item *createNode(int id,int left_stock,int price);
item *insertNode(item* root, int id,int left_stock,int price);
item *searchTree(item* root, int id);
void printTree(item* root);
void stockinit(void);
void update_stock(FILE *inputfile, item* root);

static void init_echo_cnt(void);
void echo_cnt(int connfd);
void *thread(void *vargp);
void sigint_handler(int signal);

int main(int argc, char **argv) 
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    pthread_t tid;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    stockinit();
    Signal(SIGINT,sigint_handler);

    listenfd = Open_listenfd(argv[1]);
    sbuf_init(&sbuf,SBUFSIZE);

    for(i=0;i<NTHREADS;i++)
        Pthread_create(&tid,NULL,thread,NULL);

    while (1) {
	    clientlen = sizeof(struct sockaddr_storage);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        sbuf_insert(&sbuf,connfd);

        Getnameinfo((SA *)&clientaddr,clientlen,client_hostname,MAXLINE,client_port,MAXLINE,0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
    }

    exit(0);
}
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                          /* Wait for available item */
    P(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->slots);                          /* Announce available slot */
    return item;
}
void sigint_handler(int signal){
    
    int olderrno = errno;

    FILE *fp = fopen("stock.txt", "w");
    
    if (!fp) {
        fprintf(stderr, "'stock.txt' file open error!\n");
        exit(1);
    }
    
    update_stock(fp, root);
    sbuf_deinit(&sbuf);
    Sio_puts("Update complete\n");
    fclose(fp);
    exit(0);

    errno = olderrno;
}
void *thread(void *vargp){
    Pthread_detach(pthread_self());
    while(1){
        int connfd = sbuf_remove(&sbuf);
        echo_cnt(connfd);
        Close(connfd);
    }
}

static void init_echo_cnt(void)
{
    Sem_init(&mutex, 0, 1);
    byte_cnt = 0;
}

void echo_cnt(int connfd) 
{
    int n;
    int s_id, s_cnt;
    char cmd[8];
    char response[30] = {0};
    char buf[MAXLINE]; 
    rio_t rio;
    FILE *fp = fopen("stock.txt","w");

    if(!fp){
        fprintf(stderr,"'stock.txt' file open error!\n");
        exit(1);
    }

    static pthread_once_t once = PTHREAD_ONCE_INIT;

    Pthread_once(&once, init_echo_cnt); 
    Rio_readinitb(&rio, connfd);        
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
	    sscanf(buf,"%s %d %d", cmd, &s_id, &s_cnt);
	    // byte_cnt += n; 
	    printf("Server received %d  bytes on fd %d\n",n, connfd);
        printf("cmd : %s s_id : %d s_cnt : %d\n",cmd, s_id,s_cnt);
	    // Rio_writen(connfd, buf, n); 
        if(!strcmp(cmd,"show")){
            P(&root->mutex);
            root->readcnt++;
            if(root->readcnt == 1){
                P(&root->write_mutex);
            }
            V(&root->mutex);

            printTree(root);
            Rio_writen(connfd,ans,MAXBUF);
            memset(ans,0,MAXBUF);

            P(&root->mutex);
            root->readcnt--;
            if (root->readcnt == 0) {
                V(&root->write_mutex);
            }
            V(&root->mutex);
        }
        else if(!strncmp(cmd,"buy",3)){
            struct item* ptr;
            ptr = searchTree(root,s_id);
            if (ptr) {
                P(&ptr->write_mutex);
                if (ptr->left_stock >= s_cnt) {
                    ptr->left_stock -= s_cnt;
                    snprintf(response, sizeof(response), "[buy] success\n");
                } else {
                    snprintf(response, sizeof(response), "Not enough left stocks\n");
                }
                Rio_writen(connfd, response, MAXBUF);
                memset(response,0,30);
                V(&ptr->write_mutex);
            }
        }
        else if(!strncmp(cmd,"sell",4)){
            struct item* ptr;
            ptr = searchTree(root,s_id);
            if(ptr){
                P(&ptr->write_mutex);
                ptr->left_stock += s_cnt;
                snprintf(response,sizeof(response),"[sell] success\n");
                Rio_writen(connfd,response,MAXBUF);
                memset(response,0,30);
                V(&ptr->write_mutex);
            }
        }
        else if (!strncmp(cmd, "exit", 4)) {
            snprintf(response, sizeof(response), "disconnection with server\n");
            Rio_writen(connfd, response, MAXBUF);
            memset(response, 0, 30);
            break;
        }
        memset(ans,0,MAXBUF);        
    }

}
item *createNode(int id,int left_stock,int price){
    item* node = (item*)malloc(sizeof(item));
    node->id = id;
    node->left_stock = left_stock;
    node->price = price;
    node->readcnt = 0;
    node->left = node->right = NULL;
    Sem_init(&node->mutex,0,1);
    Sem_init(&node->write_mutex,0,1);
    return node;
}
item *insertNode(item* root, int id,int left_stock,int price){
    if(root == NULL) return createNode(id,left_stock,price);
    
    if(id < root->id) root->left = insertNode(root->left,id,left_stock,price);
    else if(id > root->id) root->right = insertNode(root->right,id,left_stock,price);

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

    FILE *fp = fopen("stock.txt","r+");

    if(!fp){
        fprintf(stderr,"'stock.txt' open error!\n");
        exit(1);
    }

    while(fscanf(fp,"%d %d %d",&id, &left_stock,&price) != EOF){
        root = insertNode(root,id,left_stock,price);
    }

    fclose(fp);
    fp = NULL;
}


