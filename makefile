all:
	gcc client.c -Wall -o client
	gcc server.c -Wall -o server

#################################################################
#				  				#
# Per eseguire il programma lanciare da terminale		#
#				  				#
# Il comando 					  		#
# ./server 127.0.0.1 4000				  	#
# per eseguire il server					#
#				  				#
# Il comando						  	#
# ./client 127.0.0.1 4000				 	#
# Per lanciare il client					#
#								#
# E' necessario lanciare sempre prima il server e poi il client	#
# L'indirizzo IP e il numero di porta deve essere lo stesso 	#
#								#
#################################################################

