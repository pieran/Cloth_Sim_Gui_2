#pragma once

#include "Windows.h"
#include <string>

class ProfilingTimer
{
public:
	ProfilingTimer();
	virtual ~ProfilingTimer();

	void BeginTiming();
	void EndTiming();
	void EndTimingAdditive();

	inline void ResetTotalMs() { m_TotalMs = 0.0f; } //For Additive Profiling
	inline float GetTimedMilliSeconds() const { return m_TotalMs; }


	inline void SetAlias(const std::string& alias) { m_Alias = alias; }
	inline const std::string& GetAlias() const { return m_Alias; }
protected:
	float m_StartTime;
	float m_TotalMs;

	LARGE_INTEGER	m_CPUStart;		//Start of timer
	LARGE_INTEGER	m_CPUFrequency;	//Ticks Per Second

	std::string m_Alias;
};