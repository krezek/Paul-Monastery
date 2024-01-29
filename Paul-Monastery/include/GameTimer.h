#ifndef _GAME_TIMER_H_
#define _GAME_TIMER_H_

class GameTimer
{
public:
	GameTimer();
	~GameTimer();

	void Reset();
	void Start();
	void Stop();
	void Tick();

	float DeltaTime();
	float TotalTime();

private:
	double _secondsPerCount;
	double _deltaTime;

	__int64 _baseTime;
	__int64 _prevTime;
	__int64 _stopTime;
	__int64 _pausedTime;
	__int64 _currTime;

	bool _stopped;
};


#endif /* _GAME_TIMER_H_ */
