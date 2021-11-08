#pragma once


#include <Oblivion.h>


class Threading : public ISingletone<Threading> {
	MAKE_SINGLETONE_CAPABLE(Threading);

private:
	Threading();
	~Threading();

public:
	void ParralelForImmediate(std::function<void(int64_t)> func, int64_t count, int chunkSize);
	std::shared_ptr<struct Task> ParralelForDeffered(std::function<void(int64_t)> func, int64_t count, int chunkSize);
	void Wait(std::shared_ptr<struct Task> task);

private:
	std::vector<std::thread> m_WorkerThreads;
};

