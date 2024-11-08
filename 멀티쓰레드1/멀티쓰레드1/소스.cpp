#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>
#include <mutex>
//#include <atomic>
#include <vector>

#include <chrono>

using namespace std;

/*// ������ �̱� ������

int main()
{
	for (auto i = 0; i < 50000000; ++i) sum += 2;
	cout << "Sum =" << sum << "\n";
}
*/

// No LOCK
volatile int sum;

void thread_func()
{
	for (auto i = 0; i < 12500000; ++i) sum += 2;
}

int main()
{
	auto start = chrono::high_resolution_clock::now();

	thread t1 = thread{ thread_func };
	thread t2 = thread{ thread_func };
	thread t3 = thread{ thread_func };
	thread t4 = thread{ thread_func };

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	cout << "Sum =" << sum << "\n";
	cout << "Execution Time: " << duration << " ms\n";
}

/*// Mutex��ü�� ����Ͽ� DataRace condition�� ���� // With LOCK

mutex mylock;
volatile int sum;

void thread_func()
{
	for (auto i = 0; i < 50000000; ++i)
	{
		mylock.lock(); //mutex lock�� �ɷȴ��� Ȯ���ϱ� �ϹǷ� �ð��ҿ䰡 �ſ� ũ�� -> mips������ LL, SC�� �����ȴ�.
		// lock�����ʹ� cache�� ���� �ʰ� �޸𸮿��� �����о���Ƿ� ��������
		sum += 2;
		mylock.unlock();
	}
}

// 
int main()
{
	auto start = chrono::high_resolution_clock::now();

	thread t1 = thread{ thread_func };
	//thread t2 = thread{ thread_func };
	//thread t3 = thread{ thread_func };
	//thread t4 = thread{ thread_func };

	t1.join();
	//t2.join();
	//t3.join();
	//t4.join();

	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	cout << "Sum =" << sum << "\n";
	cout << "Execution Time: " << duration << " ms\n";
}*/

/*// atomic�� ����
volatile std::atomic<int> sum(0);

void ThreadFunc(int num_threads)
{
	for (int i = 0; i < 50000000 / num_threads; i++)
		_asm lock add sum, 2; //sum�� 2�� ���ϴ� �������� ������ atomic���� ���� ���� ����ũ�θ���� �ܰ踦 ���ľ������� atomic�� �������� �����Ѵ�
}

int main()
{
	int num_threads = 4;

	auto start = chrono::high_resolution_clock::now();

	thread t1 = thread{ ThreadFunc, num_threads };
	thread t2 = thread{ ThreadFunc, num_threads };
	thread t3 = thread{ ThreadFunc, num_threads };
	thread t4 = thread{ ThreadFunc, num_threads };

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	cout << "Sum =" << sum << "\n";
	cout << "Execution Time: " << duration << " ms\n";
}*/

/*//Data Race�� ���� ���

mutex mylock;
volatile int num;

void optimal_thread_func(int num_threads)
{
	volatile int local_sum = 0;
	for (auto i = 0; i < 50000000 / num_threads; ++i)
	{
		local_sum += 2;
	}
	mylock.lock();
	num += local_sum;
	mylock.unlock();
}

int main()
{
	vector<thread*> threads;

	int num_threads = 1;

	auto start = chrono::high_resolution_clock::now();

	thread t1 = thread{ optimal_thread_func, num_threads };
	//thread t2 = thread{ optimal_thread_func, num_threads };
	//thread t3 = thread{ optimal_thread_func, num_threads };
	//thread t4 = thread{ optimal_thread_func, num_threads };

	t1.join();
	//t2.join();
	//t3.join();
	//t4.join();

	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

	cout << "Sum =" << num << "\n";
	cout << "Execution Time: " << duration << " ms\n";
}*/
