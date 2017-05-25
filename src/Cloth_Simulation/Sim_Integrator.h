#pragma once
#include <glcore\Vector3.h>
#include <functional>
#include "SimulationDefines.h"
#include "Sim_Generator.h"
#include "ProfilingTimer.h"

#define DEFAULT_SUB_TIMESTEP 0.0005f//(1.f / 920.f)

enum Sim_Integrator_Type
{
	Sim_Integrator_Type_Explicit = 0,
	Sim_Integrator_Type_RK2 = 1,
	Sim_Integrator_Type_RK4 = 2
};

class Sim_Integratable
{
public:
	//Returns true if successful or false for explosion
	virtual bool StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt) = 0;
};

class Sim_Integrator
{
public:
	Sim_Integrator();
	virtual ~Sim_Integrator();

	void Initialize(Sim_Integratable* m_Sim, const Sim_Generator_Output& configuration);
	void UpdateSimulation(float real_timestep);

	void SetSubTimestep(float sub_timestep) { m_SubTimestep = sub_timestep; }
	float& GetSubTimestep() { return m_SubTimestep; }

	Sim_Integrator_Type& GetIntergrationType() { return m_IntegrationType; }
	void SetIntegrationType(Sim_Integrator_Type type);
	float& GetElapsedTime() { return m_TimeElapsedTotal; }
	int GetNumSolverCalls() { return m_NumSolverCalls; }

	void SetOnStepCompleteCallback(const std::function<void(Sim_Integrator*)>& callback) { m_CallbackOnStepComplete = callback; }

	ProfilingTimer& GetTotalTimer() { return m_ProfilingTotalTime; }

	Vector3* X() { return m_X; }
	Vector3* DxDt() { return m_DxDt; }

	const Vector3& GetGravity() { return m_Gravity; }
	void SetGravity(const Vector3& gravity) { m_Gravity = gravity; }

protected:
	void InitIntegrationTypeMem();

	void InitializeMemExplicit();
	bool IntegrateExplicit();

	void InitializeMemRK2();
	bool IntegrateRK2();


	void InitializeMemRK4();
	bool IntegrateRK4();

protected:
	std::function<void(Sim_Integrator*)> m_CallbackOnStepComplete;
	std::function<void()> m_FuncInitMem;
	std::function<bool()> m_FuncIntegrate;

	Vector3 m_Gravity;

	float m_SubTimestep;
	float m_TimeAccum;
	float m_TimeElapsedTotal;
	Sim_Integratable* m_Sim;

	Sim_Integrator_Type m_IntegrationType;

	unsigned int m_NumTotal;
	Vector3* m_X;
	Vector3* m_DxDt;

	Vector3* m_X_Tmp;
	Vector3* m_DxDt_Tmp;	

	int m_NumSolverCalls;
	ProfilingTimer m_ProfilingTotalTime;

	const std::vector<std::function<void(float elapsed, Vector3* x, Vector3* dxdt)>>* m_Actuators;
};