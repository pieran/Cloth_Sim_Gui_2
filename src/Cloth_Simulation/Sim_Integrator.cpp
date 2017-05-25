#include "Sim_Integrator.h"
#include <glcore\NCLDebug.h>

Sim_Integrator::Sim_Integrator()
	: m_X(NULL)
	, m_DxDt(NULL)
	, m_DxDt_Tmp(NULL)
	, m_X_Tmp(NULL)
	, m_Actuators(NULL)
{
	m_SubTimestep = DEFAULT_SUB_TIMESTEP;

	m_ProfilingTotalTime.SetAlias("Total Time");
	m_NumSolverCalls = 0;

	m_Gravity = Vector3(0.f, -9.81f, 0.f);

	m_IntegrationType = Sim_Integrator_Type_Explicit;
	m_FuncInitMem = std::bind(&Sim_Integrator::InitializeMemExplicit, this);
	m_FuncIntegrate = std::bind(&Sim_Integrator::IntegrateExplicit, this);
}

Sim_Integrator::~Sim_Integrator()
{
	m_Actuators = NULL;

	if (m_X != NULL)
	{
		delete[] m_X;
		delete[] m_DxDt;
		m_X = NULL;
	}

	if (m_X_Tmp != NULL)
	{
		delete[] m_X_Tmp;
		m_X_Tmp = NULL;
	}

	if (m_DxDt_Tmp != NULL)
	{
		delete[] m_DxDt_Tmp;
		m_DxDt_Tmp = NULL;
	}
}

void Sim_Integrator::SetIntegrationType(Sim_Integrator_Type type)
{
	if (type != m_IntegrationType)
	{
		m_IntegrationType = type;

		switch (m_IntegrationType)
		{
		case Sim_Integrator_Type_Explicit:
			m_FuncInitMem = std::bind(&Sim_Integrator::InitializeMemExplicit, this);
			m_FuncIntegrate = std::bind(&Sim_Integrator::IntegrateExplicit, this);
			break;

		case Sim_Integrator_Type_RK2:
			m_FuncInitMem = std::bind(&Sim_Integrator::InitializeMemRK2, this);
			m_FuncIntegrate = std::bind(&Sim_Integrator::IntegrateRK2, this);
			break;

		case Sim_Integrator_Type_RK4:
			m_FuncInitMem = std::bind(&Sim_Integrator::InitializeMemRK4, this);
			m_FuncIntegrate = std::bind(&Sim_Integrator::IntegrateRK4, this);
			break;
		}

		InitIntegrationTypeMem();
	}
}

void Sim_Integrator::InitIntegrationTypeMem()
{
	if (m_X_Tmp != NULL) delete[] m_X_Tmp;
	if (m_DxDt_Tmp != NULL) delete[] m_DxDt_Tmp;

	m_X_Tmp = NULL;
	m_DxDt_Tmp = NULL;

	m_FuncInitMem();
}

void Sim_Integrator::Initialize(Sim_Integratable* sim, const Sim_Generator_Output& config)
{
	
	m_Sim = sim;
	m_NumSolverCalls = 0;
	m_TimeAccum = 0.f;
	m_TimeElapsedTotal = 0.f;
	m_NumTotal = config.Phyxels.size();
	
	m_Actuators = &config.Actuators;

	if (m_X != NULL) delete[] m_X;
	if (m_DxDt != NULL) delete[] m_DxDt;
	

	m_X = new Vector3[m_NumTotal];
	m_DxDt = new Vector3[m_NumTotal];
	
	memset(m_DxDt, 0, m_NumTotal * sizeof(Vector3));
	memcpy(m_X, &config.Phyxels[0], m_NumTotal * sizeof(Vector3));

	auto new_type = Sim_Integrator_Type_Explicit;
	new_type = Sim_Integrator_Type_RK2;
	//new_type = Sim_Integrator_Type_RK4;
	if (new_type != m_IntegrationType)
	{
		SetIntegrationType(new_type);
	}
	else
	{
		InitIntegrationTypeMem();
	}
}

void Sim_Integrator::UpdateSimulation(float real_timestep)
{
	m_NumSolverCalls = 0;

	m_ProfilingTotalTime.BeginTiming();
	m_TimeAccum += real_timestep;
	for (; m_TimeAccum - m_SubTimestep >= 0.f; m_TimeAccum -= m_SubTimestep)
	{
		if (m_Actuators != NULL)
		{
			for (int i = 0; i < m_Actuators->size(); ++i)
				(*m_Actuators)[i](m_TimeElapsedTotal, m_X, m_DxDt);
		}

		m_FuncIntegrate();

		m_TimeElapsedTotal += m_SubTimestep;
	}
	m_ProfilingTotalTime.EndTiming();

	if (m_CallbackOnStepComplete)
		m_CallbackOnStepComplete(this);
}



#pragma region EXPLICIT_INTEGRATION

void Sim_Integrator::InitializeMemExplicit()
{
	m_DxDt_Tmp = new Vector3[m_NumTotal];
	memset(m_DxDt_Tmp, 0, m_NumTotal * sizeof(Vector3));
}

bool Sim_Integrator::IntegrateExplicit()
{
	m_NumSolverCalls++;

	if (m_Sim->StepSimulation(m_SubTimestep, m_Gravity, m_X, m_DxDt, m_DxDt_Tmp))
	{
		std::swap(m_DxDt, m_DxDt_Tmp);

#pragma omp parallel for
		for (int i = 0; i < m_NumTotal; ++i)
		{
			m_X[i] += m_DxDt[i] * m_SubTimestep;
		}
	}

	return true;
}

#pragma endregion //EXPLICIT_INTEGRATION



#pragma region RK2_INTEGRATION

void Sim_Integrator::InitializeMemRK2()
{
	m_DxDt_Tmp = new Vector3[m_NumTotal];
	m_X_Tmp = new Vector3[m_NumTotal];

	memset(m_DxDt_Tmp, 0, m_NumTotal * sizeof(Vector3));
	memset(m_X_Tmp, 0, m_NumTotal * sizeof(Vector3));
}

bool Sim_Integrator::IntegrateRK2()
{
	const float half_timestep = m_SubTimestep * 0.5f;
	
#pragma omp parallel for
	for (int i = 0; i < m_NumTotal; ++i)
	{
		m_X_Tmp[i] = m_X[i] + m_DxDt[i] * half_timestep;		
	}

	m_NumSolverCalls++;
	if (m_Sim->StepSimulation(half_timestep, m_Gravity, m_X, m_DxDt, m_DxDt_Tmp))
	{
		std::swap(m_DxDt, m_DxDt_Tmp);

#pragma omp parallel for
		for (int i = 0; i < m_NumTotal; ++i)
		{
			m_X[i] += m_DxDt[i] * m_SubTimestep;
		}
	}

	return true;
}

#pragma endregion RK2_INTEGRATION


#pragma region RK4_INTEGRATION

void Sim_Integrator::InitializeMemRK4()
{
	m_DxDt_Tmp = new Vector3[m_NumTotal * 3];
	m_X_Tmp = new Vector3[m_NumTotal];

	memset(m_DxDt_Tmp, 0, m_NumTotal * 3 * sizeof(Vector3));
	memset(m_X_Tmp, 0, m_NumTotal * sizeof(Vector3));
}

bool Sim_Integrator::IntegrateRK4()
{
	const float half_timestep = m_SubTimestep * 0.5f;

	Vector3* k1 = m_DxDt;
	Vector3* k2 = m_DxDt_Tmp;
	Vector3* k3 = &m_DxDt_Tmp[m_NumTotal];
	Vector3* k4 = &m_DxDt_Tmp[m_NumTotal * 2];


#pragma omp parallel for
	for (int i = 0; i < m_NumTotal; ++i)
	{
		m_X_Tmp[i] = m_X[i] + k1[i] * half_timestep;
	}
	m_NumSolverCalls++;
	m_Sim->StepSimulation(half_timestep, m_Gravity, m_X_Tmp, k1, k2);

#pragma omp parallel for
	for (int i = 0; i < m_NumTotal; ++i)
	{

		m_X_Tmp[i] = m_X[i] + k2[i] * half_timestep;
	}
	m_NumSolverCalls++;
	m_Sim->StepSimulation(half_timestep, m_Gravity, m_X_Tmp, k2, k3);

#pragma omp parallel for
	for (int i = 0; i < m_NumTotal; ++i)
	{
		m_X_Tmp[i] = m_X[i] + k3[i] * m_SubTimestep;
	}
	m_NumSolverCalls++;
	m_Sim->StepSimulation(m_SubTimestep, m_Gravity, m_X_Tmp, k3, k4);

#pragma omp parallel for
	for (int i = 0; i < m_NumTotal; ++i)
	{
		m_DxDt[i] = (k1[i] + (k2[i] + k3[i]) * 2.f + k4[i]) / 6.f;
		m_X[i] += m_DxDt[i] * m_SubTimestep;
	}

	return true;
}

#pragma endregion RK4_INTEGRATION