//MASTERMIND SERVER

#include <sys/select.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>


#define DIM 50
#define FALSE 0
#define TRUE 1
#define WHO 1
#define NICK_OK 2
#define NICK_NO 3
#define LOGIN 4
#define CONNECT 5
#define GAME_REQ 6
#define RX_NO 9
#define RX_OK 10
#define IAMFREE 13

struct connection{
	int clsk;//descrittore del connected socket restituito dalla accept
	struct sockaddr_in add;
	socklen_t len;
	struct connection *next;
};


struct client_elem{
		int clsk;//descrittore del connected socket restituito dalla accept
		char *nickname;
		int UDP_port;
		int occupato;
		struct client_elem *next;
};

struct client_elem *lista_client=NULL;		//lista dei client connessi
struct connection *connection_list=NULL;	//lista delle connessioni attive
fd_set master;

//mi limito ad inserire in testa alla lista
void inserimento_lista(struct client_elem *new_elem){
	new_elem->next=lista_client;
	lista_client=new_elem;
	return;
}

//elenca i client connessi attualmente
void who(int i){
		struct client_elem *punt=lista_client; //prendo la testa della lista
		int size_mess;
		int command=0;
			while(punt!=NULL){//la scorro tutta
				size_mess=strlen(punt->nickname)+1;
				send(i,&size_mess,sizeof(int),0);//invio dimensione
				send(i,punt->nickname,size_mess,0);//invio nome client
				/*send occupato,opzionale*/
				command=punt->occupato;
				send(i,&command,sizeof(int),0);//invio lo stato del client(occupato/libero)
				punt=punt->next;			
			}
			command=0;//invio il comando 0 a segnalare che ho finito di inviare
			send(i,&command,sizeof(int),0);
			
}

/*La funzione search restituisce 0 se non trova corrispondenza,1 se la trova,
ma il giocatore è occupato,e punt->skcl altrimenti*/

int search(char *buff){
	struct client_elem *punt=lista_client;
	while(punt!=NULL){//scorro tutta la lista
		if(strcmp(buff,punt->nickname)==0){//se lo trovo
			if(punt->occupato==0)return punt->clsk;//e non è occupato ritorno il socket
			else return 1;}
		punt=punt->next;
	}
	return 0;
}

//provo ad eseguire il login
int login(int i){
	struct client_elem *new_client=calloc(1,sizeof(struct client_elem));
	new_client->clsk=i;
	new_client->occupato=FALSE;
	int size_mess,code;
	recv(i,&size_mess,sizeof(int),0);
	char *buff=calloc(size_mess,sizeof(char));
	recv(i,buff,size_mess,0);
	//la funzione search cerca se esiste un nick uguale
	if(search(buff)==0){//se non presente lo inserisco
		code=NICK_OK;
		send(i,&code,sizeof(int),0);//invio codice che il nick va bene
		new_client->nickname=buff;
		recv(i,&new_client->UDP_port,sizeof(int),0);//ricevo porta
		inserimento_lista(new_client);
		printf("Nuovo giocatore:%s si è connesso\n",buff);
		printf("%s è LIBERO\n",buff);
		fflush(stdout);
		return 0;
	}
	else{
					printf("giocatore non valido\n");
					code=NICK_NO;//invio il codice che il nick non va bene
					send(i,&code,sizeof(int),0);
					return 1;
		}			    
																		
}
//sfidato i, sfidante from
void invio_richiesta(int i,int from){
	struct client_elem *p=lista_client;
	struct connection *pp=connection_list;
	int code=GAME_REQ;
	int trovato=0;
	int len;
	char buff[DIM];
	int port_to_send;
	struct sockaddr_in to_send;
	send(i,&code,sizeof(int),0);//invio codice richiesta sfida
	/*dire il nome di from*/
	while(p!=NULL && !trovato){
		if(p->clsk==from){
			trovato=1;
			strcpy(buff,p->nickname);
		}
		p=p->next;
	}
	len=strlen(buff)+1;
	send(i,&len,sizeof(int),0);//invio dimensione
	send(i,&buff,len,0);//invio nick
	
	trovato=0;
	p=lista_client;
	//cerco la porta scorrendo la lista dei client
	while(p!=NULL && !trovato){
		if(p->clsk==from){
			trovato=1;
			port_to_send=p->UDP_port;
		}
		p=p->next;
	}
	
	trovato=0;
	//cerco se lo trovo nella lista delle connessioni
	while(pp!=NULL && !trovato){
		if(pp->clsk==from){
			trovato=1;
			to_send=pp->add;
		}
		pp=pp->next;
	}
	
	len=sizeof(to_send);//invio
	send(i,&port_to_send,sizeof(int),0);//porta
	send(i,&len,sizeof(int),0);//dimensione
	send(i,&to_send,len,0);//socket a cui inviare(sfidato)
	send(i,&from,sizeof(int),0);//socket di provenienza (sfidante)
}

//ho come paramentro il socket
void my_connect(int i){
	int size_mess,ris;
	char buff[DIM];
	recv(i,&size_mess,sizeof(int),0);//ricevo dimensione
	recv(i,buff,size_mess,0);//ricevo nickname
	ris=search(buff);//0 non esiste,1 occupato,oppure enemy_sk ok
	if(i==ris) ris=2;//errore,vuole giocare con se stesso
	send(i,&ris,sizeof(int),0);//invio risultato della ricerca
	if(ris==0 || ris==1) return;
	invio_richiesta(ris,i);//invia richiesta di sfida
	
}

//dato socket scorre la lista dei client e restituisce il nickname
char* int_to_nick(int i){
	struct client_elem *p=lista_client;
	while(p!=NULL){
		if(p->clsk==i){
			return p->nickname;
		}
		p=p->next;
	}
	return NULL;
}

/*i=da chi proviene la rix,j=chi ha fatto la richiesta*/
void req_rx(int code,int i){
	int j,len;
	int trovato=0;
	struct client_elem *p=lista_client;
	struct connection *pp=connection_list;
	int port_to_send;
	struct sockaddr_in to_send;
	char *buff1;
	char *buff2;
	recv(i,&j,sizeof(int),0);//ricevo &enemy_sk da game_req(), è il socket dello sfidato
	if(code==RX_NO){
		send(j,&code,sizeof(int),0);//invio codice di risposta a chi ha fatto la richiesta
		return;
	}
	
	/*code RX_OK*/
	send(j,&code,sizeof(int),0);//invio codice di risposta a chi ha fatto la richiesta
	//cerco la posta scorrendo la lista dei client
	while(p!=NULL && !trovato){
		if(p->clsk==i){
			trovato=1;
			port_to_send=p->UDP_port;
		}
		p=p->next;
	}
	
	trovato=0;
	//cerco se lo trovo nella lista delle connessioni
	while(pp!=NULL && !trovato){
		if(pp->clsk==i){
			trovato=1;
			to_send=pp->add;
		}
		pp=pp->next;
	}

	len=sizeof(to_send);//invio
	send(j,&port_to_send,sizeof(int),0);//porta
	send(j,&len,sizeof(int),0);//dimensione
	send(j,&to_send,len,0);//indirizzo
	
	p=lista_client;
	while(p!=NULL){//setto entrambi ad occupato
		if((p->clsk==j) || (p->clsk==i)){
			p->occupato=TRUE;
		}
		p=p->next;
	}
	//ottengo il nick dal socket
	buff1=int_to_nick(j);
	buff2=int_to_nick(i);
	printf("%s si è connesso a %s\n",buff1,buff2);
}

//funzione che controlla chi è libero
void iamfree(int i){
	struct client_elem *p=lista_client;
	int trovato=0;
	char buff[DIM];
	while(p!=NULL && !trovato){
		if(p->clsk==i) {p->occupato=0;
		strcpy(buff,p->nickname);
		trovato=1;}
		p=p->next;
	}
	if(trovato) printf("%s è LIBERO\n",buff);	
}

int disconnetti(int i){
	struct client_elem *p=NULL;
	struct client_elem *q;
	struct connection *pp=NULL;
	struct connection *qq;
	char name[DIM];
	/*set 0 in master[i]*/
	FD_CLR(i,&master);//rimuovo descrittore da quelli da controllare
	
	//rimuovo client da lista_client del server
	for (q = lista_client; q != NULL && q->clsk != i; q = q->next)
		p = q;
	/*esce dal for*/
	if (q == 0) return FALSE;
	if (q == lista_client) 
		lista_client = q->next; 
	else 
		p->next= q->next;
	strcpy(name,q->nickname);
	free(q);

	//rimuovo connessione client da connection_list del server		
	for (qq = connection_list; qq != NULL && qq->clsk != i; qq = qq->next)
		pp = qq;
	/*esce dal for*/
	if (qq == 0) return FALSE;
	if (qq == connection_list) 
		connection_list = qq->next; 
	else 
		pp->next= qq->next;	
	free(qq);
	
	printf("%s si è disconesso dal server\n",name);
	return TRUE;

}

//paramentri: docie indice fs_set				
int read_code(int code,int i){
	switch(code){
		case WHO:
			who(i);
			return 1;
			break;
		case LOGIN:
			login(i);
			return 1;
			break;	
		case CONNECT:
			my_connect(i);
			return 1;
			break;
		case IAMFREE://se client ha vinto/perso o si è disconnesso
			iamfree(i);
			return 1;
			break;
		case RX_NO: case RX_OK:
			req_rx(code,i);
			return 1;
		default: return 0;
		}
						
}


int main(int argc,char** argv){
	/*VARIABILI E STRUTTURE*/
	/*se gli argomenti non sono 3,esce */
	if (argc!=3){
            fprintf(stderr, "Usare: udp-client <host> <port>\n");
            exit(1);
	}

	int ris,i;
	int sk_server,clsk,fd_max;
	int code;
	fd_set read_fds;
	struct sockaddr_in addr_server;
	memset(&addr_server, 0, sizeof(addr_server));
	int yes=1;
	int r;	
	/*PROGRAMMA*/
	 
	addr_server.sin_family = AF_INET; 

	ris=inet_pton(AF_INET,argv[1],&addr_server.sin_addr.s_addr);
	/*converte la stringa inserita(IP) in formato "network" */
	if(ris!=1) {printf("errore nell'indirizzo IP del server\n");exit(1);}
	
	ris=atoi(argv[2]);
	if(ris<1023 || ris>65535) {printf("errore nella porta del server\n");exit(1);}
	addr_server.sin_port=htons(ris);
	/*converte la porta(data come stringa) in intero,che sarà convertito in network*/
	if((sk_server=socket(PF_INET,SOCK_STREAM,0))==-1) {
		printf("Errore sulla creazione del socket TCP\n");exit(1);
	}
	
	if(setsockopt(sk_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
		perror("Creazione socket TCP fallita!");
     		exit(1);
	}

	
	if(bind(sk_server,(struct sockaddr *)&addr_server,sizeof(addr_server))==-1){
		printf("Errore sulla bind\n");
		perror("BIND:");exit(1);
	}
	if(listen(sk_server,10)==-1){/* la listen vuole come parametri il socket
	 di ascolto e il num massimo di richieste pendenti*/
		printf("Errore sulla listen\n");exit(1);
	}
	
	printf("Indirizzo:%s (Porta: %s)\n",argv[1],argv[2]);
	
	/*INIZIALIZZAZIONE SELECT*/
	
	FD_ZERO(&master);//inizializzo i set
	FD_ZERO(&read_fds);//inizializzo i set
	FD_SET(sk_server,&master);//setto il bit relativo a sk_server
	fd_max=sk_server;
	for(;;){
		/*la select vuole come primo parametro il massimo descrittore+1,
		e come ultimo il timeout.Ritorna <0 se errore,=0 se è scaduto
		 il timeout,e >0 se è andata a buon fine,restituendo il num di
		 descrittori settati.Modifica il campo read_fds opportunamente.*/
		read_fds=master;//faccio una copia del set dei descrittori da controllare perchè la select modifica i set di input
		if((ris=select(fd_max+1,&read_fds,NULL,NULL,NULL))<0){
			printf("Errore sulla select\n");
		}
		
		for(i=0;i<=fd_max;i++){
			if(FD_ISSET(i,&read_fds)){
			/*se arriviamo qui vuol dire che ci sono dati sul socket i-esimo*/
			    if(i==sk_server){
				/*se il socket su cui sono arrivati i dati è quello di ascolto,
				allora dobbiamo fare una accept e stabilire una nuova 
				connessione*/
	
				/*************************************************************/
				size_t leng=sizeof(struct connection);
				struct connection *new_conn=calloc(1,leng);
				/*memset(&myadd,0,sizeof(myadd));*/
				new_conn->add.sin_family=AF_INET;//IPv4
				socklen_t len=sizeof(new_conn->add);	
				/**************************************************************/
	
	
				if((clsk=accept(sk_server,(struct sockaddr *)&new_conn->add,&len))==-1){
					printf("Errore nella accept\n");
				}
				else{
					new_conn->clsk=clsk;
					new_conn->len=len;
					new_conn->next=connection_list;
					connection_list=new_conn;
					printf("CONNESSIONE ACCETTATA\n");
					if(clsk>fd_max)fd_max=clsk;
						FD_SET(clsk,&master);			 	
				}
				
			    }
			    else{/*LA RICHIESTA ARRIVA DA UN CLIENT GIA' CONNESSO*/
				    printf("In attesa di CODE\n");
				    r=recv(i,&code,sizeof(int),0);//ricevo codice da un client già connesso
				    if(r==-1) printf("Errore nella recv\n");
				    else{
					if(r==0) r=disconnetti(i);
					else read_code(code,i);
			 	    }
			     }
			}
		}
	
	/*RITORNO AL FOR*/
	}
		
	return 0;
}
