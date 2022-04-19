#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <condition_variable>

template<typename T>
class ThreadPool {
private:
	int thread_number_;
	int max_requests_;
	std::vector<std::thread> threads_;
	std::list<T*> work_queue_;
	std::mutex queue_locker_;
	std::condition_variable cond_;
	ConnectionPool* conn_pool_;
	int actor_model_;

	static void* Worker(void* arg);
	void Run();
public:
	ThreadPool(int actor_model, ConnectionPool* coon_pool, int thread_number = 8, int max_request = 10000);
	bool Append(T* request, int state);
	bool Append_p(T* request);
};
template <typename T>
ThreadPool<T>::ThreadPool(int actor_model, ConnectionPool* coon_pool, int thread_number, int max_requests) :
	actor_model_(actor_model), thread_number_(thread_number), max_requests_(max_requests), conn_pool_(connPool)
{
	if (thread_number <= 0 || max_requests <= 0)
		throw std::exception();
	for (int i = 0; i < thread_number; ++i)
	{
		threads_.emplace_back(Worker, reinterpret_cast<void*> (this));
		threads_[i].detach();
	}
}
template <typename T>
bool ThreadPool<T>::Append(T* request, int state)
{
	std::lock_guard<mutex> t(queue_locker_);
	if (work_queue_.size() >= max_requests_)return false;

	request->m_state = state;
	work_queue_.push_back(request);
	cond_.notify_one();
	return true;
}
template <typename T>
bool ThreadPool<T>::Append_p(T* request)
{
	std::lock_guard<mutex> t(queue_locker_);
	if (work_queue_.size() >= max_requests_)return false;

	work_queue_.push_back(request);
	cond_.notify_one();
	return true;
}
template <typename T>
void* ThreadPool<T>::Worker(void* arg)
{
	auto* pool = reinterpret_cast<ThreadPool*>(arg);
	pool->Run();
	return reinterpret_cast<void*>(pool);
}
template <typename T>
void ThreadPool<T>::Run()
{
	while (true)
	{

		std::unique_lock<mutex> lock(queue_locker_);
		cond_.wait(lock);
		if (m_workqueue.empty())continue;

		T* request = work_queue_.front();
		work_queue_.pop_front();
		lock.unlock();
		if (!request)continue;

		if (1 == actor_model_)
		{
			if (0 == request->m_state)
			{
				if (request->read_once())
				{
					request->improv = 1;
					connectionRAII mysqlcon(&request->mysql, m_connPool);
					request->process();
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
			else
			{
				if (request->write())
				{
					request->improv = 1;
				}
				else
				{
					request->improv = 1;
					request->timer_flag = 1;
				}
			}
		}
		else
		{
			connectionRAII mysqlcon(&request->mysql, conn_pool_);
			request->process();
		}
	}
}