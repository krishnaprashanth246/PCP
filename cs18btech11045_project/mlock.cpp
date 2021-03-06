#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <stdlib.h>
#include <chrono>
#include <mutex>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>
#include <random>

using namespace std;

int n,k; //number of threads and number of cs entry requests for each thread
double lambda1,lambda2; //parameters for exponential distribution of time
ofstream fout; //output file object

double ave_enter;//to measure times
double ave_exit;
double wor_enter;
double wor_exit;


class QNode//additional node structure containing flag
{
public:
	bool locked;
	QNode()
	{
		this->locked = false;
	}
};

class slock //our shared lock object
{
public:
	QNode* L;
	int id;
	slock() {
		L = new QNode();
		id = -1;//id < 0
	}
};


class mlock
{
public:
	atomic<slock*> sharedlock;//atomic pointer

	mlock() {//initialize
		slock* temp = new slock;
		atomic_init(&sharedlock, temp);
	};
	// I, K, P, L are the variables in pseudo code
	// I  is myNode
	// K is myPred
	// P is pred
	// **a means a->locked
	void m_acquire( QNode* myNode, QNode* myPred, int myid )
	{
		int old_id;
		myNode->locked = true;//acquire

		slock* temp = new slock;
		temp->L = myNode;
		temp->id = myid;

		QNode* pred;
		slock* req;
		req = sharedlock.exchange(temp);
		pred = req->L;
		old_id = req->id;

		if(old_id > 0) {//if there is no queue
			if (!myPred) {// if myPred is not NULL
				delete[] myPred;//free it
			}
		}
		while (pred->locked != 0) /* spin */ ; 
	}

	void m_release( QNode* myNode, QNode* myPred, int myid ) {
		int success = 0;
		myNode->locked = false;

		slock* rep = new slock;
		rep->L  = myNode;
		rep->id = myid;

		slock* newrep = new slock;
		newrep->L = rep->L;
		newrep->id = 0;
		//compare exchange returns true only if rep and sharedlock are equal
		//we actually need to compare only ids but if id is equal myid, then it
		//implies that rep->L is also equal to myNode. So checking whole node is ok
		success = sharedlock.compare_exchange_weak(rep, newrep);
		if (!success)
			if (!myPred) {
				myNode = myPred;
				myPred = NULL;
			} 
			else
				myNode = new QNode;
	}
	~mlock();
	
};


void testCS(int id,mlock* Test,default_random_engine generator,exponential_distribution<double> distribution1,exponential_distribution<double> distribution2)
{
	std::chrono::time_point<std::chrono::system_clock> bc1,ec1,bc2,ec2;

	for(int i=0;i<k;i++)
	{
		
		QNode* myNode = new QNode();
		QNode* myPred = new QNode();

		std::time_t start1 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		string reqEnterTime = ctime(&start1);
		reqEnterTime = reqEnterTime.substr(11,8);
		fout<<i+1<<" th CS Entry Request at "<< reqEnterTime << " by thread "<< id <<endl;

		
		bc1 = std::chrono::system_clock::now();
		Test->m_acquire(myNode, myPred,id);
		ec1 = std::chrono::system_clock::now();
		

		std::time_t end1 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		string actEnterTime = ctime(&end1);
		actEnterTime = actEnterTime.substr(11,8);
		fout<<i+1<<" th CS Entry at "<< actEnterTime <<" by thread "<< id <<endl;

		unsigned int t1 = distribution1(generator);
		sleep(t1);

		std::time_t start2 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		string reqExitTime = ctime(&start2);
		reqExitTime = reqExitTime.substr(11,8);
		fout<<i+1<<" th CS Exit Request at "<< reqExitTime <<" by thread "<< id <<endl;


		bc2 = std::chrono::system_clock::now();
		Test->m_release(myNode, myPred,id);
		ec2 = std::chrono::system_clock::now();


		std::time_t end2 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		string actExitTime = ctime(&end2);
		actExitTime = actExitTime.substr(11,8);
		fout<<i+1<<" th CS Exit at "<< actExitTime <<" by thread "<< id <<endl;

		std::chrono::duration<double> tc1 = ec1 - bc1;
		std::chrono::duration<double,std::micro> tc2 = ec2 - bc2;
		
		ave_enter += tc1.count();
		ave_exit += tc2.count();

		wor_enter = max(tc1.count(),wor_enter);
		wor_exit = max(tc2.count(),wor_exit);


		unsigned int t2 = distribution2(generator);
		sleep(t2);

		
	}
}

void runlock()
{
	int seed = chrono::system_clock::now().time_since_epoch().count();
	default_random_engine generator(seed);
	exponential_distribution<double> distribution1(1.0/lambda1);
	exponential_distribution<double> distribution2(1.0/lambda2);

	thread thread_pool[n];

	mlock* Test = new mlock();

	for(long long int i=0;i<n;i++)
		thread_pool[i] = thread(testCS,i+1,Test,generator,distribution1,distribution2);

	for(long long int i=0;i<n;i++)
		thread_pool[i].join();


// For calculation of average times

	double dur1,dur2;
	dur1 = ave_enter;
	dur1 = dur1/(double)(n*k);
	dur2 = ave_exit;
	dur2 = dur2/(double)(n*k);
	ave_enter = 0;
	ave_exit = 0;
	// cout<<"Time :\n";
	// cout<<"average enter for "<<n<<" is "<<dur1<<endl;
	// cout<<"average exit for "<<n<<" is "<<dur2<<endl;
	// cout<<"worst entry for "<<n<<" is "<<wor_enter<<endl;
	// cout<<"worst exit for "<<n<<" is "<<wor_exit<<endl;


}

int main()
{
	ifstream read_file("inp-params.txt");  //input file

	read_file>>n>>k>>lambda1>>lambda2;
	read_file.close();
	fout.open("output-mlock.txt");
	//cout<<"gtlock Lock Output:"<<endl;
	runlock();
	fout.close();

}
