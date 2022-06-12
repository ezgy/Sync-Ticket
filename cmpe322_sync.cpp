#include <iostream>
#include <fstream>
#include <pthread.h>
#include <array>
#include <vector>
#include <unistd.h>
#include <string>
#include <bits/stdc++.h>
#include <semaphore.h>
#include <queue>

/*
* Name: Ezgi Aysel Batı
* Student No: 2018400207
* Course: Cmpe 322: Operating Systems
* Project: Ticket Reservation Simulation with Thread Synchronization
* 
*/

using namespace std;

ofstream outfile;
bool *seating; //this is the seating plan of the theater hall
pthread_mutex_t mutex1; //file writing mutex
pthread_mutex_t mutex2; //file writing mutex
pthread_mutex_t tellermutex; //teller operation mutex

queue<string> sharedclientinfo; //clients put their information here, teller read from here
int clientcount,size; 
sem_t teller;
sem_t customer;

void *clientfunct(void *args){ //function for client threads
	//Get client info
	string a;
	string infoline = *((string*)args);
	stringstream clientinfo(infoline);
	string name,temp;
	int arrival, service, desiredSeat;
	getline(clientinfo,name,',');
	getline(clientinfo,temp,',');
	arrival = stoi(temp);
	usleep(arrival*100); //wait until arrival time
	sem_wait(&teller);
	pthread_mutex_lock(&tellermutex); //mutex queues requests
	sharedclientinfo.push(infoline); //client information is passed onto teller via shared variable
	pthread_mutex_unlock(&tellermutex); //teller is freed 
	sem_post(&customer);
	pthread_exit(NULL);
}

void *tellerfunct(void *threadid){ //function for teller threads
	//Initialization
	
	char& tn = *((char*) threadid); 
	pthread_mutex_lock(&mutex1);
	outfile <<"Teller " <<tn<<" has arrived." <<endl; 
	pthread_mutex_unlock(&mutex1);
	sem_post(&teller);
	sleep(1); //For proper order of tellers
	while(1){
		
		if(clientcount<=0){
			pthread_exit(NULL);
			
		}
		sem_wait(&customer);
		//Read and parse client info
		stringstream clientinfo(sharedclientinfo.front());
		sharedclientinfo.pop();
		string name,temp;
		int arrival, service, desiredSeat;
		getline(clientinfo,name,',');
		getline(clientinfo,temp,',');
		arrival = stoi(temp);
		getline(clientinfo,temp,',');
		service = stoi(temp);
		getline(clientinfo,temp,',');
		desiredSeat = stoi(temp);

		pthread_mutex_lock(&mutex1);
		//Check if requested seat is available
		if(seating[desiredSeat-1]==false){//seat available
			seating[desiredSeat-1]=true;
			
			outfile <<name <<" requests seat " <<desiredSeat <<", reserves seat " <<desiredSeat <<". Signed by Teller " <<tn <<"." <<endl;
		}else{//check for any available seats
			bool placed = false;
			for(int i=0; i<size; i++){
				if(seating[i]==false){//give this seat
					seating[i]=true;
					placed = true;
					outfile <<name <<" requests seat " <<desiredSeat <<", reserves seat " <<(i+1) <<". Signed by Teller "<<tn <<"." <<endl;
					break;
				}
			}
			if(!placed){
				outfile <<name <<" requests seat " <<desiredSeat <<", reserves seat None. Signed by Teller "<<tn <<"." <<endl;
				
			}  
		}
		pthread_mutex_unlock(&mutex1);
		usleep(service*100);
		sem_post(&teller);
		
		pthread_mutex_lock(&mutex2);
		clientcount-=1;
		pthread_mutex_unlock(&mutex2);
	}
}

int main(int argc, char *argv[]){
	pthread_mutex_init(&mutex1, NULL);
	pthread_mutex_init(&mutex2, NULL);
	pthread_mutex_init(&tellermutex,NULL);

	sem_init(&customer, 0, 0);
	sem_init(&teller, 0, 3);
	string out_file= argv[2];
	outfile.open(out_file);
	
	string theaterName;
	ifstream infile(argv[1]);
	infile >> theaterName;
	infile >> clientcount;
	int org = clientcount;
	if(theaterName.compare("OdaTiyatrosu")==0)
		size = 60;
	else if(theaterName.compare("UskudarTiyatroSahne")==0)
		size = 80;
	else //Kucuk Sahne
		size = 200;
		
	seating = new bool[size]; //initialize hall according to size
	fill(seating,seating+size,false); //initially all false-empty, switch to true as seats fill up.
	
	pthread_mutex_lock(&mutex1);
	outfile <<"Welcome to the Sync-Ticket!" <<endl;
	pthread_mutex_unlock(&mutex1);

	//create teller threads
	pthread_t tellers[3];
	char tellernames[3] = {'A','B','C'};
	for(int i = 0; i < 3; i++) {
        	pthread_create(&tellers[i], NULL,&tellerfunct, (void*) &tellernames[i]);
        	sleep(1);
    	}
    	string temp;
    	getline(infile,temp);

	//create client threads
	pthread_t clients[org];
	string infolines[org];
	for(int i=0; i<org;i++){
		getline(infile,temp);
		infolines[i]=temp;
		pthread_create(&clients[i],NULL,&clientfunct,&infolines[i]);
	}
	
	//main thread stops only when all clients are served so all tellers finish.
	//Tellers remain active until all clients are served
	for(int j = 0; j < org; j++) {
	   pthread_join(clients[j], NULL);
	}
	sleep(1);
	pthread_mutex_lock(&mutex1);
	outfile <<"All clients recieved service." <<endl;
	pthread_mutex_unlock(&mutex1);
	
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&tellermutex);
	sem_destroy(&customer);
	sem_destroy(&teller);
	
	return 0;	
}





