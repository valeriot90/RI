// MASTERMIND CLIENT

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
#define TRUE 1
#define FALSE 0
#define WHO 1
#define NICK_OK 2
#define LOGIN 4
#define CONNECT 5
#define GAME_REQ 6
#define RX_NO 9
#define RX_OK 10
#define COMB 11
#define GG 12
#define IAMFREE 13
#define HELP "Sono disponibili i seguenti comandi:\n*  !help  -->  mostra  l'elenco  dei  comandi  disponibili\n*  !who  -->  mostra  l'elenco  dei  client  connessi  al  server\n*  !connect  nome_client  -->  avvia  una  partita  con  l'utente  nome_client\n*  !disconnect  -->  disconnette  il  client  dall'attuale  partita  intrapresa con un altro peer\n*  !combinazione comb  -->  permette al client di fare un tentativo con la combinazione comb\n*  !quit  -->  disconnette  il  client  dal  server\n"

int esatti;	//numero cifre esatte al posto giusto
int presenti;	//numero cifre esatte al posto sbagliato
int sk_client;	//socket del client
int in_game=0; 	// 0=not in game,1=in game with X
int port_udp_enemy;	//porta udp avversario
struct sockaddr_in enemy_addr;	//struttura indirizzo avversario
char codicesegreto[5];  	 //combinazione segreta del client
char codice[5];	        	 //buffer dove inserire la combinazione che inserisco
int mio_turno;	//specifica se è il mio turno 0 oppure no 1
int waiting=0;	//in attesa di una risposta di richiesta se 1
int sk_udp;	//il mio socket
char enemy[DIM];//nome avversario
int i,j;	//indici

//funzione che mi permettere di vedere chi è connesso
void who(){
	char buffer[DIM];
	int code=WHO;
	int size_mess;
	send(sk_client,&code,sizeof(int),0);//invia codice ad indicare richiesta elenco utenti
	recv(sk_client,&size_mess,sizeof(int),0);//ricevo dimensione messaggio
	printf("Client connessi al server:\n");
	
	while(size_mess!=0){
		recv(sk_client,&buffer,size_mess,0);//ricevo nome
		printf("%s",buffer);
		recv(sk_client,&code,sizeof(int),0);//ricevo stato utente
		if(code==0)printf(" LIBERO\n");
		else printf(" OCCUPATO\n");
		recv(sk_client,&size_mess,sizeof(int),0);//ricevo dim==0->esco da while, non ho altri dati da ricevere
		}
}


//provo a collegarmi, attendo la risposta dal server
void my_connect(){
	int z;
	char buff[DIM];
	int dim,code;
	scanf("%50s",buff);
	dim=strlen(buff)+1;
	/*invio code connect*/
	code=CONNECT;
	send(sk_client,&code,sizeof(int),0);//invio codice che voglio connettermi
	/*invio dati*/
	for(z=0; z<DIM; z++)
		enemy[z]=buff[z];//risolve BUG: chi si collega per primo al server e sfida client non vede nome avversario
	send(sk_client,&dim,sizeof(int),0);//invio dimensione
	send(sk_client,&buff,dim,0);//invio nick
	/*aspetta conferma*/
	recv(sk_client,&code,sizeof(int),0);//ricevo codice per vedere se il nick è stato accettato o no
	if(code==0){
		printf("IMPOSSIBILE CONNETTERSI:%s non valido\n",buff);
		return;
		}
	if(code==1){
		printf("IMPOSSIBILE CONNETTERSI:%s occupato\n",buff);
		return;
		}
	if(code==2){
		printf("Sei tu %s!! Vuoi davvero giocare da solo?!?!\n",buff);
		return;
	}
	printf("Invio richiesta di connessione,attendere\n");
	waiting=1;
	/*sarà il server a "svegliarci"*/
	
}
//////////////////////////////////////////////////////////////////////////

void game_req(){
	/*dire chi ha inviato la richiesta*/
	int dim,enemy_sk;
	int code;
	recv(sk_client,&dim,sizeof(int),0);
	recv(sk_client,&enemy,dim,0);
	recv(sk_client,&port_udp_enemy,sizeof(int),0);
	recv(sk_client,&dim,sizeof(int),0);
	recv(sk_client,&enemy_addr,dim,0);
	recv(sk_client,&enemy_sk,sizeof(int),0);
	enemy_addr.sin_port=htons(port_udp_enemy);
	char cc='a';
	fflush(stdout);
	fflush(stdin);
	printf("hai una richiesta per una partita da %s,accettare(s/n)?\n",enemy);
	scanf(" %c",&cc);
	while(cc!='s' && cc!='n'){
	printf("hai una richiesta per una partita da %s,accettare(s/n)?\n",enemy);
	scanf(" %c",&cc);
	}
	if(cc=='n'){code=RX_NO;
	 	send(sk_client,&code,sizeof(int),0);//invio codice
	 	send(sk_client,&enemy_sk,sizeof(int),0);//invio socket nemico
	 	printf("richiesta rifiutata\n"); return;}
	 /*c==s*/
	else{
          	 code=RX_OK;
		 send(sk_client,&code,sizeof(int),0);//invio codice
		 send(sk_client,&enemy_sk,sizeof(int),0);//invio socket nemico
		 printf("richiesta accettata\n");
		 fflush(stdin);
		 fflush(stdout);
		 do{
			printf("\nDigita la tua combinazione segreta di 4 cifre: ");
			fflush(stdout);
			scanf("%s",codicesegreto);
		}while((strlen(codicesegreto)) != 4);
		printf("Giochi per secondo\n");		 
		in_game=1;
		mio_turno=0;
	}
}

//vedo risposta alla richiesta di sfida
void rix(int code){
	int dim;
	waiting=0;
	if(code==RX_NO){
		printf("richiesta rifiutata\n");
		return;
	}
	else{
		printf("richiesta accettata!\n");//ricevo
		recv(sk_client,&port_udp_enemy,sizeof(int),0);//porta
		recv(sk_client,&dim,sizeof(int),0);//dimensione
		recv(sk_client,&enemy_addr,dim,0);//indirizzo
		enemy_addr.sin_port=htons(port_udp_enemy);//converto little-endian->big-endian
		do{
			printf("\nDigita la tua combinazione segreta di 4 cifre: ");
			fflush(stdout);
			scanf("%s",codicesegreto);
		}while((strlen(codicesegreto)) != 4);
		printf("Inizi a giocare per primo \n");
		in_game=1;
		mio_turno=1;
		}
}

//chiamata se ho vinto/perso o mi sono disconnesso
void gg(){
	int code;
	socklen_t len=sizeof(enemy_addr);
	recvfrom(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,&len);//ricevo codice
	if(code==0)printf("\nHai perso!!\n\n");		
	else {
		printf("\n %s si è arreso!!!\n",enemy);
		printf("HAI VINTO!!!\n");
	}
	
	code=IAMFREE;
	send(sk_client,&code,sizeof(int),0);//invio codice al server che adesso sono libero
	fflush(stdout);
	in_game=0;
	
}

void combinazione(){
	int code;
	int buff[2];
	int r;
	do{	fflush(stdout);
		fflush(stdin);
		scanf("%s",codice);
	}while((strlen(codice)) != 4);
	code=COMB;//invio codice e la combinazione
	r=sendto(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
	if(r<0)printf("Errore nella sendto (code)");
	sendto(sk_udp,&codice,sizeof(codice),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
	if(r<0)printf("Errore nella sendto (combinazione)");
	
	socklen_t len=sizeof(enemy_addr);//ricevo il buffer di 2 interi per occorrenze della sequenza
	recvfrom(sk_udp,&buff,sizeof(buff),0,(struct sockaddr*)&enemy_addr,&len);

	if(buff[0]==4){
		printf("\nHAI VINTO!!!\n\n");
		/*invio dati sia all' enemy che al server*/
		code=GG;//invio codice
		sendto(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
		code=0;//invio segnale che ho vinto
		sendto(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
		code=IAMFREE;//invio al server che ora sono libero
		send(sk_client,&code,sizeof(int),0);
		in_game=0;
	}
	else{
		printf("%s dice:\n cifre giuste al posto giusto %d,\n"
       	 		 " cifre giuste al posto sbagliato %d\n",enemy,buff[0],buff[1]);
	}
	mio_turno=(mio_turno+1)%2;
}


void combinazione_recv(){
	int buff[2];
	socklen_t len=sizeof(enemy_addr);//ricevo la combinazione
	recvfrom(sk_udp,&codice,sizeof(codice),0,(struct sockaddr*)&enemy_addr,&len);
	esatti=0;
        presenti=0;
	//controllo quanti numeri della combinazione ha indovinato o sono presenti
	for(j=0;j<4;j++){
		if(codice[j]==codicesegreto[j])
			esatti++;
		for(i=0; i<4; i++){
			if((j != i) && (codice[j]==codicesegreto[i]))
				presenti++;
		}
	}
	buff[0]=esatti;
	buff[1]=presenti;
	if(esatti==4)
		printf("%s dice: %d%d%d%d Ha indovinato la combinazione!\n",enemy,codice[0]-48, codice[1]-48, codice[2]-48, codice[3]-48);		
	else	
		printf("%s dice: %d%d%d%d Il suo tentativo è sbagliato\n", enemy, codice[0]-48, codice[1]-48, codice[2]-48, codice[3]-48);
	//invio resoconto all'avversario
	sendto(sk_udp,&buff,sizeof(buff),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
	printf("E' il tuo turno:\n");
	mio_turno=(mio_turno+1)%2;
}

void my_disconnect(){
	int code;
	code=GG;//codice di disconnessione
	sendto(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
	code=1;//invio codice che ho perso(mi sono arreso)
	sendto(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,sizeof(enemy_addr));
	code=IAMFREE;//invio al server che ora sono libero
	send(sk_client,&code,sizeof(int),0);
	in_game=0;
	printf("TI SEI ARRESO!!!\n");
}

//mi disconnetto dalla partita, poi mi disconnetto dal server e termino
void my_quit(int a){
	if(a==1)my_disconnect();
	close(sk_client);
	close(sk_udp);
	exit(0);
}

/*********************************************************************/

int main(int argc,char** argv){
	/*variabili*/
	/*se gli argomenti non sono 3,esce */
	if (argc!=3){
            fprintf(stderr, "Usare: udp-client <host> <port>\n");
            exit(1);
    	}

	int ris,len,code=-1;
	char nickname[DIM],command[DIM];
	int udp_port;
	int nick_valido=FALSE;
	int fd_max;
	int r;
	struct timeval timeout;
	struct sockaddr_in addr_server;
	struct sockaddr_in my_add_udp;
	memset(&addr_server, 0, sizeof(addr_server));
	memset(&nickname,'\0', DIM);
	
	
	addr_server.sin_family = AF_INET; //IPv4

	ris=inet_pton(AF_INET,argv[1],&addr_server.sin_addr.s_addr);
	/*converte la stringa inserita(IP) in formato "network" */
	if(ris!=1) {printf("errore nell'indirizzo IP del server\n");exit(1);}
	
	ris=atoi(argv[2]);
	if(ris<1023 || ris>65535) {printf("errore nella porta del server\n");exit(1);}
	addr_server.sin_port=htons(ris);
	/*converte la porta(data come stringa) in intero,che sarà convertito in network*/
	
	sk_client=socket(AF_INET,SOCK_STREAM,0);
	
	ris=connect(sk_client,(const struct sockaddr *) &addr_server,sizeof(addr_server));
	if(ris!=0) {printf("errore durante la connessione\n"); exit(1);}
	printf("Connessione al server %s (porta %s) effettuata con successo\n\n",argv[1],argv[2]);

	printf("%s\n", HELP);	

	do{
		printf("Inserisci il tuo nick-name:\n");
		scanf("%50s",nickname); //ATTENZIONE!!! FARE CONTROLLO SCANF
	while (getchar()!='\n');
	//send del nickname e controllo 
	len=strlen(nickname)+1;//prelevo il nome
	code=LOGIN;//invio codice che voglio loggarmi
	send(sk_client,&code,sizeof(int),0);
	send(sk_client,&len,sizeof(int),0);//invio lunghezza
	send(sk_client,nickname,len,0);//invio il nickname
	recv(sk_client,&code,sizeof(int),0);//ricevo codice se il nick è stato accettato oppure no
	if(code==NICK_OK)nick_valido=TRUE;
	else printf("Nick-name non valido\n");
	//si setta nick_valido a TRUE
	}
	while(!nick_valido);
		printf("Nick-name valido!\n");

	do{
		if(udp_port > 65535) printf("Valore inserito non valido. Riprova.\n");
	        printf("Inserisci la tua porta UDP di ascolto (>1023): ");
	        scanf("%d",&udp_port);
	        while (getchar()!='\n');
    	}
	    while(udp_port<=1023 || udp_port > 65535);
	send(sk_client,&udp_port,sizeof(int),0);//invio porta


	sk_udp=socket(AF_INET,SOCK_DGRAM,0);
	memset(&my_add_udp, 0, sizeof(my_add_udp));
	my_add_udp.sin_family = AF_INET;
	my_add_udp.sin_port = htons(udp_port);
	my_add_udp.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sk_udp, (struct sockaddr*) &my_add_udp, sizeof(my_add_udp)) == -1) {
		perror("bind");
		exit(1);
	}


	printf("In ascolto sulla porta UDP:%d\n",udp_port);

	fd_set master; //creazione fd_set
//(insieme dei descrittori da controllare pronti per la lettura
//sono array di interi in cui ogni bit rappresenta un descrittore)	
	fd_set read_fds; //creazione fd_set 
	FD_ZERO(&master); //inizializzo
	FD_ZERO(&read_fds); //inizializzo
	FD_SET(0,&master);//mette a 1 il bit 0(il primo)
	FD_SET(sk_client,&master);//voglio controllare il socker sk_client, quindi setto il relativo bit
	fd_max=sk_client;
	FD_SET(sk_udp,&master);//mette a 1 il bit sk_udp
	if(sk_udp>sk_client) fd_max=sk_udp;
//la select modifica gli insiemi di descrittori puntati da read_set.
//Ritorna negli insiemi interessati (indicati nella select) i descrittori che sono pronti
for(;;){
	//faccio una copia del set dei descrittori da controllare perchè la select modifica i set di input
	read_fds=master;
	printf("Inserisci un comando(!help per la lista)\n");
	if(in_game==0)printf("> "); else printf("# ");
	fflush(NULL);
	
	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
	
	if((ris=select(fd_max+1,&read_fds,NULL,NULL,&timeout))<0){
			printf("errore sulla select\n");
		}
	if(ris==0) {printf("TIMEOUT SCADUTO\n");
		if(in_game!=0) my_disconnect();}
	int i;
	for(i=0;i<=fd_max;i++){//aggiungo nuovo descrittore tra quelli da controllare da ora in avanti
			if(FD_ISSET(i,&read_fds)){
	//se il descrittore pronto è quello di 0 allora significa che ho la richiesta di connessione pendente e posso fare la accept
					if(i==0){
						/*STDIN*/
						r=0;
						scanf("%s",command);
						if(strcmp(command,"!help")==0)
							{printf("%s\n", HELP);;r=1;}
						if(strcmp(command,"!who")==0) 
							{who();r=1;}
						if(strcmp(command,"!connect")==0){ 
							if(!in_game && !waiting){
								my_connect();r=1;}
							else if(in_game) printf("Stai giocando! Sei già collegato ad un client!");
						}
						if((strcmp(command,"!combinazione")==0)){
							if(in_game && mio_turno){
								combinazione();
								r=1;}
							else if(!in_game) printf("Non stai giocando!");
							else if(!mio_turno) printf("Non è il tuo turno!");
						}
						if((strcmp(command,"!disconnect")==0)){							
							if(in_game && mio_turno){
								my_disconnect();
								r=1;}
							else if(!in_game) printf("Non stai giocando!");
							else if(!mio_turno) printf("Non è il tuo turno!");
						}

						if(strcmp(command,"!quit")==0){
							if(in_game)
								my_quit(1);
							else my_quit(0);
							r=1;
						}						
						if(r==0)printf("\n Comando non valido\n");
						fflush(stdout);
					}
					else{
						if(i==sk_client){
//se il descrittore pronto è quello di sk_client allora significa 
//che ho la richiesta di connessione pendente e posso fare la accept
						/*RISPOSTA DAL SERVER*/
						r=recv(sk_client,&code,sizeof(int),0);
						if(r==-1)printf("errore nella recv\n");
						if(r==0) {printf("SERVER DOWN\n");exit(1);}
						if(code==GAME_REQ) game_req();
						if(code==RX_NO || code==RX_OK)rix(code);
						}
						else{/*proviene dal socket udp*/
							socklen_t len=sizeof(enemy_addr);
							r=recvfrom(sk_udp,&code,sizeof(int),0,(struct sockaddr*)&enemy_addr,&len);
							if(r==-1)printf("errore");
							if(r==0){printf("Client down\n");
									 in_game=0;}
							if(code==COMB) combinazione_recv();
							if(code==GG) gg();
						
						
						}
					}	
	         }
	}
}

return 0;
}
