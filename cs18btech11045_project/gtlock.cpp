#include<bits/stdc++.h>
#include<atomic>
#include<ctime>
#include<chrono>
#include<mutex>
#include<thread>
#include<fstream>
#include<random>
#include<sys/time.h>
#include<unistd.h>

using namespace std;

int n,k;//number of threads and number of cs entry requests for each thread
double lambda1, lambda2;//parameters for exponential distribution of time
ofstream fout;//output file object

double ave_enter;//to measure times
double ave_exit;
double wor_enter;
double wor_exit;


typedef struct thinfo {//helper struct
    int who_was_last;
    char this_means_locked;
}thinfo;

typedef struct qlock {/* the lock */
    atomic<char*> bytes;
    atomic<thinfo> info;
}qlock;



class gtlock// The Granuke and Thakkar's Lock
{
public:
    qlock shared;
    gtlock(){//initialize everything
        thinfo tempinfo;
        tempinfo.who_was_last = -1;
        tempinfo.this_means_locked = '0';
        atomic_init(&shared.info,tempinfo);
        char* tempbytes = (char*)malloc(n*sizeof(char));//have an array of flags for each thread
        for(int i=0; i<n; i++) {
            tempbytes[i] = '0';
        }
        atomic_init(&shared.bytes, tempbytes);
    };

    void to_lock(int myid) {

        thinfo myinfo;

        myinfo.who_was_last = myid;
        char* allbits = shared.bytes.load();
        //getting the bytes initially is okay because my location is updated only by me
        myinfo.this_means_locked = allbits[myid];
        myinfo = shared.info.exchange(myinfo);//linearization point

        while (shared.bytes.load()[myinfo.who_was_last]== myinfo.this_means_locked)
            ;
    }

    void to_unlock(int myid)  {
        char* allbits = shared.bytes.load();
        
		if(allbits[myid] == '0') {//flip
			allbits[myid] = '1';
		}
		else {
			allbits[myid] = '0';
		};
        shared.bytes.store(allbits);
    }

    ~gtlock();
};


void testCS(int id,gtlock* Test,default_random_engine generator,exponential_distribution<double> distribution1,exponential_distribution<double> distribution2)
{
	std::chrono::time_point<std::chrono::system_clock> bc1,ec1,bc2,ec2;

	for(int i=0;i<k;i++)
	{
		
		std::time_t start1 = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		string reqEnterTime = ctime(&start1);
		reqEnterTime = reqEnterTime.substr(11,8);
		fout<<i+1<<" th CS Entry Request at "<< reqEnterTime << " by thread "<< id <<endl;

		
		bc1 = std::chrono::system_clock::now();
		Test->to_lock(id);
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
		Test->to_unlock(id);
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

void lock()
{
	int seed = chrono::system_clock::now().time_since_epoch().count();
	default_random_engine generator(seed);
	exponential_distribution<double> distribution1(1.0/lambda1);
	exponential_distribution<double> distribution2(1.0/lambda2);

	thread thread_pool[n];

	gtlock* Test = new gtlock();

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
	// cout<<"Time :\n ";
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

	fout.open("output-gtlock.txt");
	fout<<"gtlock Lock Output:"<<endl;
	lock();
	fout.close();
}
