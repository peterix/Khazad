#ifndef TIMER__HEADER
#define TIMER__HEADER

#include <stdafx.h>


class Timer
{
public:

	Timer(Uint32 Size = 1);
	~Timer();

	void Start();
	const Uint32 Stop();

	void Pause();
	void Unpause();

	void SetSampleSize(Uint32 Size);

	const Uint32 getElapsed();

	void doAverage();
    float getAverage();

protected:

	Uint32 StartTime;
	Uint32 PausedTime;

    Uint32 SampleSize;
    Uint32 SampleIndex;

    Uint32 SamplingPause;
    float AverageTime;

    Uint32* AcumulationArray;

	bool started;
	bool paused;
};

#endif // TIMER__HEADER
