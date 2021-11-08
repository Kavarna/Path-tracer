#include "Threading.h"


struct Task {
	Task(std::function<void(int64_t)> func, int64_t maxIndex, int chunckSize) :
		function1D(func), maxIndex(maxIndex), chunckSize(chunckSize), taskID(TaskID) { };

	static unsigned long long TaskID;

	std::function<void(int64_t)> function1D;
	int64_t maxIndex;
	int64_t nextIndex = 0;
	std::shared_ptr<Task> nextTask = nullptr;
	int chunckSize;
	int activeWorkers = 0;
	unsigned long long taskID;

	bool Finished() {
		return nextIndex >= maxIndex && activeWorkers == 0;
	}
};

unsigned long long Task::TaskID = 0;

std::mutex workListMutex;
std::condition_variable workerThreadConditionVariable;
std::atomic_bool shouldClose = false;
std::shared_ptr<Task> workList = nullptr;

std::set<std::shared_ptr<Task>> finishedTasks;


void workerThreadFunc(unsigned int threadIndex) {
	Oblivion::DebugPrintLine("Starting thread ", threadIndex);

	std::unique_lock<std::mutex> lock(workListMutex);
	while (true) {
		if (!shouldClose && workList == nullptr) {
			Oblivion::DebugPrintLine("Thread ", threadIndex, " starts waiting");
			workerThreadConditionVariable.wait(lock);
			Oblivion::DebugPrintLine("Thread ", threadIndex, " is waking up");
		}

		if (shouldClose) {
			Oblivion::DebugPrintLine("Thread ", threadIndex, " is shutting down...");
			break;
		} else {
			if (workList == nullptr)
				continue;

			auto myTask = workList;

			int64_t indexStart = myTask->nextIndex;
			int64_t indexEnd = std::min(indexStart + myTask->chunckSize, myTask->maxIndex);

			myTask->nextIndex = indexEnd;
			if (indexEnd == myTask->maxIndex) {
				finishedTasks.insert(myTask);
				workList = myTask->nextTask;
			}
			myTask->activeWorkers++;

			lock.unlock();

			for (auto i = indexStart; i < indexEnd; ++i) {
				if (myTask->function1D) {
					myTask->function1D(i);
				}
			}

			lock.lock();
			myTask->activeWorkers--;
			if (myTask->Finished()) {
				workerThreadConditionVariable.notify_all();
			}
		}
	}
}

Threading::Threading() {
	auto numThreads = std::max(1u, std::thread::hardware_concurrency());
	m_WorkerThreads.reserve(numThreads);

	for (unsigned int i = 0; i < numThreads; ++i) {
		m_WorkerThreads.emplace_back(workerThreadFunc, i + 1);
	}
}

Threading::~Threading() {
	shouldClose = true;
	workerThreadConditionVariable.notify_all();
	for (auto& th : m_WorkerThreads) {
		th.join();
	}
}

void Threading::ParralelForImmediate(std::function<void(int64_t)> func, int64_t count, int chunkSize) {
	if (count < chunkSize) {
		for (int64_t i = 0; i < count; ++i) {
			func(i);
		}
	}

	std::shared_ptr<Task> currentTask = std::make_shared<Task>(std::move(func), count, chunkSize);
	workListMutex.lock();
	currentTask->nextTask = workList;
	workList = currentTask;
	workListMutex.unlock();

	std::unique_lock<std::mutex> lock(workListMutex);
	workerThreadConditionVariable.notify_all();
	while (!currentTask->Finished()) {
		int64_t indexStart = currentTask->nextIndex;
		int64_t indexEnd = std::min(indexStart + currentTask->chunckSize, currentTask->maxIndex);

		currentTask->nextIndex = indexEnd;
		if (indexEnd == currentTask->maxIndex) {
			finishedTasks.insert(currentTask);
			workList = currentTask->nextTask;
		}
		currentTask->activeWorkers++;

		lock.unlock();

		for (auto i = indexStart; i < indexEnd; ++i) {
			if (currentTask->function1D) {
				currentTask->function1D(i);
			}
		}

		lock.lock();
		currentTask->activeWorkers--;
	}
}

std::shared_ptr<Task> Threading::ParralelForDeffered(std::function<void(int64_t)> func, int64_t count, int chunkSize) {
	std::shared_ptr<Task> currentTask = std::make_shared<Task>(std::move(func), count, chunkSize);
	workListMutex.lock();
	currentTask->nextTask = nullptr;
	auto listIterator = workList;
	if (workList != nullptr) {
		while (workList->nextTask != nullptr);
		workList->nextTask = currentTask;
	} else {
		workList = currentTask;
	}
	workListMutex.unlock();

	workerThreadConditionVariable.notify_all();

	return currentTask;
}

void Threading::Wait(std::shared_ptr<struct Task> currentTask) {
	if (finishedTasks.find(currentTask) == finishedTasks.end()) {
		// The task is not ready yet, come to the resque!
		std::unique_lock<std::mutex> lock(workListMutex);
		workerThreadConditionVariable.notify_all();
		while (!currentTask->Finished()) {
			int64_t indexStart = currentTask->nextIndex;
			int64_t indexEnd = std::min(indexStart + currentTask->chunckSize, currentTask->maxIndex);

			currentTask->nextIndex = indexEnd;
			if (indexEnd == currentTask->maxIndex) {
				finishedTasks.insert(currentTask);
				workList = currentTask->nextTask;
			}
			currentTask->activeWorkers++;

			lock.unlock();

			for (auto i = indexStart; i < indexEnd; ++i) {
				if (currentTask->function1D) {
					currentTask->function1D(i);
				}
			}

			lock.lock();
			currentTask->activeWorkers--;
		}
	}
}
