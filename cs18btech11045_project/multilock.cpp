#include<bits/stdc++.h>
#include<atomic>
#include<ctime>
#include<chrono>
#include<mutex>
#include<thread>
#include<fstream>
#include<random>
#include<bitset>

using namespace std;

int n,k;//number of threads and number of cs entry requests for each thread
double lambda1, lambda2;//parameters for exponential distribution of time
ofstream fout;//output file object

double ave_enter;//to measure times
double ave_exit;
double wor_enter;
double wor_exit;

void multitest();
void multitestCS (int id);

typedef struct cell {
	atomic<unsigned int> seq;// sequence number
	bitset<32> bits;
}cell;

typedef struct mrlock { 
	cell *buffer;
	unsigned int mask ;
	atomic <unsigned int> head ;
	atomic <unsigned int> tail ;
}mrlock;

class Qnode
{
public:
	bool locked ;
	Qnode() {
		locked = false;
	}
};

class Multilock
{
	mrlock l;//shared lock object
public:
	Multilock ( unsigned int siz ) {
		l.buffer = new cell[siz];
		l.mask = siz - 1;
		atomic_init(&l.head,0u);
		atomic_init(&l.tail,0u);
		// initialize bits to all 1s and seq to cell index
		for ( unsigned int i = 0; i < siz ; i ++)
		{
			l.buffer[i].bits.set();
			atomic_init(&l.buffer[i].seq,i);
		}
	}

	~Multilock () {
		delete[] l.buffer ;
	}

	unsigned int acquire_lock ( bitset<32> r ) {
		cell * c ;
		unsigned int pos ;

		while(true) {// cas loop , try to increase tail
			pos = l.tail.load (memory_order_relaxed );
			//cout<<"in acquire_lock inside "<<pos<<endl;
			c = & l.buffer [ pos & l.mask ];
			unsigned int seq = c -> seq.load (memory_order_acquire );
			int dif = (int) seq - ( int) pos ;
			if ( dif == 0) {
				if ( l.tail.compare_exchange_weak(pos, pos + 1 ,memory_order_relaxed ))
					break ;
			}
		}
		c -> bits = r ; // update the cell content
		c -> seq.store ( pos + 1 ,memory_order_release );
		unsigned int spin = l.head ;
		while ( spin != pos ) {
			if ( pos - l.buffer[spin & l.mask].seq > l.mask || ( l.buffer[spin & l.mask].bits& r).flip().any())
				spin ++;
		}
		//cout<<"in acquire_lock "<<pos<<endl;
		return pos ;
	};
	void release_lock ( unsigned int h){
		l.buffer [h & l.mask].bits.reset();
		unsigned int pos = l.head.load (memory_order_relaxed );
		//cout<<"in release_lock "<<pos<<endl;

		while ( l.buffer[pos & l.mask].bits== 0) {
			cell *c = & l . buffer [ pos & l.mask ];
			unsigned int seq = c->seq.load (memory_order_acquire );
			int dif = ( int ) seq - ( int) ( pos + 1) ;

			if ( dif == 0) {
				if (l.head.compare_exchange_weak(pos, pos + 1,memory_order_relaxed)){
					c -> bits.set();
					c -> seq.store(pos + l.mask+ 1,memory_order_release );
				}
			}
			pos = l.head.load(memory_order_relaxed );
		}
	}

};



//shared lock objects for all threads
Multilock *MultiTest;

//shared duration variables
//atomic<double> aggwaitenter, aggwaitexit;

int main(int argc, char const *argv[])
{
	ifstream fin;
	fin.open("inp-params.txt");//getting input parameters
	fin >> n >> k >> lambda1 >> lambda2;
	fin.close();

	int seed =  chrono::system_clock::now().time_since_epoch().count();
	default_random_engine generator (seed);
	exponential_distribution<double> distribution1(1/lambda1);
	exponential_distribution<double> distribution2(1/lambda2); 


	fout.open("output-multilock.txt");
	fout<<"Multi resource Lock Output:"<<endl;

	multitest();
	// cout<<"Average enter time: "<<aggwaitenter.load()/(n*k*1e6)<<" sec"<<endl;
	// cout<<"Average exit time: "<<aggwaitexit.load()/(n*k)<<" micro sec"<<endl;//for graph purpose
	fout.close();

	return 0;
}

void multitest()
{
	vector<std::thread> thread_pool;
	thread_pool.reserve(n);
	MultiTest = new Multilock(n);
	for (int i = 0; i < n; i++)
	{   
		thread_pool.push_back(std::thread(multitestCS, i));
	}//each thread has its unique identifier passed by value through i
	for (int i = 0; i < n; i++)
	{   
		thread_pool[i].join();
	}//main thread waiting for other threads

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

	delete MultiTest;
}

void multitestCS (int id)
{
	default_random_engine generator;//random generator source
	exponential_distribution<double> distribution1(1/lambda1);
	exponential_distribution<double> distribution2(1/lambda2);

	std::chrono::time_point<std::chrono::system_clock> bc1,ec1,bc2,ec2;

	//structs
	bitset<32> myres;
	myres.set();
	unsigned int pos;

	for ( int i = 0; i < k ; i++)
	{
		auto reqEnterTimenow = chrono::system_clock::now();
		auto reqEnterTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
		string printreqEnterTime = ctime(&reqEnterTime);//get full date and time
		if(printreqEnterTime[printreqEnterTime.length()-1] == '\n') 
			printreqEnterTime[printreqEnterTime.length()-1] = '\0';//removing newline at the end of time string
		printreqEnterTime = printreqEnterTime.substr(printreqEnterTime.length()-14, 8);//getting only time
		
		fout << i << " th CS Entry Request at " << printreqEnterTime << " by thread " << id <<endl;
		

		bc1 = std::chrono::system_clock::now();
		pos = MultiTest->acquire_lock(myres);
		ec1 = std::chrono::system_clock::now();

		auto actEnterTimenow = chrono::system_clock::now();
		auto actEnterTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
		string printactEnterTime = ctime(&actEnterTime);//get full date and time
		if(printactEnterTime[printactEnterTime.length()-1] == '\n') 
			printactEnterTime[printactEnterTime.length()-1] = '\0';//removing newline at the end of time string
		printactEnterTime = printactEnterTime.substr(printactEnterTime.length()-14, 8);//getting only time
				

		fout << i << " th CS Entry at " << printactEnterTime << " by thread " << id <<endl;

		double T1 = distribution1(generator); 
		chrono::duration<double> period1 (T1);

		this_thread::sleep_for (period1);//sleep

		auto reqExitTimenow = chrono::system_clock::now();
		auto reqExitTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
		string printreqExitTime = ctime(&reqExitTime);//get full date and time
		if(printreqExitTime[printreqExitTime.length()-1] == '\n') 
			printreqExitTime[printreqExitTime.length()-1] = '\0';//removing newline at the end of time string
		printreqExitTime = printreqExitTime.substr(printreqExitTime.length()-14, 8);//getting only time

		fout << i << " th CS Exit Request at " << printreqExitTime << " by thread " << id <<endl;

		bc2 = std::chrono::system_clock::now();
		MultiTest->release_lock(pos) ;
		ec2 = std::chrono::system_clock::now();

		auto actExitTimenow = chrono::system_clock::now();
		auto actExitTime = chrono::system_clock::to_time_t(chrono::system_clock::now());
		string printactExitTime = ctime(&actExitTime);//get full date and time
		if(printactExitTime[printactExitTime.length()-1] == '\n') 
			printactExitTime[printactExitTime.length()-1] = '\0';//removing newline at the end of time string
		printactExitTime = printactExitTime.substr(printactExitTime.length()-14, 8);//getting only time
		
		std::chrono::duration<double> tc1 = ec1 - bc1;
		std::chrono::duration<double,std::micro> tc2 = ec2 - bc2;
		
		ave_enter += tc1.count();
		ave_exit += tc2.count();

		wor_enter = max(tc1.count(),wor_enter);
		wor_exit = max(tc2.count(),wor_exit);

		fout << i << " th CS Exit at " << printactExitTime << " by thread " << id <<endl;
		double T2 = distribution2(generator); 
		chrono::duration<double> period2 (T2);

		this_thread::sleep_for (period2);//sleep
	}
	//delete myNode;
}
