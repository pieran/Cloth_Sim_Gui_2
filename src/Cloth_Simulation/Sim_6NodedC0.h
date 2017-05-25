#pragma once


#include "mpcg.h"

#include "EigenDefines.h"
#include "SimulationDefines.h"

#include <glcore\Matrix4.h>
#include <glcore\Matrix3.h>
#include <glcore\Vector3.h>
#include <glcore\Vector2.h>
#include "ProfilingTimer.h"

#include "Sim_Renderer.h"
#include "Sim_Integrator.h"
#include "Sim_Manager.h"

#define USE_DYNAMIC_MINMAX FALSE



typedef Eigen::Matrix<float, 12, 12> StiffnessMatrix;

typedef Eigen::Matrix<float, 3, 18> BMatrix;
typedef Eigen::Matrix<float, 2, 2>  JaMatrix;
typedef Eigen::Matrix<float, 6, 18> GMatrix;
typedef Eigen::Matrix<float, 2, 6> Mat26;
typedef Eigen::Matrix<float, 18, 1> VDisplacement;

enum Sim_6Noded_SubTimer
{
	Sim_6Noded_SubTimer_Rotations = 0,
	Sim_6Noded_SubTimer_BuildMatrices,
	Sim_6Noded_SubTimer_Solver,
	Sim_6Noded_SubTimer_MAX
};

class Sim_6NodedC0 : public Sim_Simulation, Sim_Rendererable, Sim_Integratable
{
	friend class ClothRenderObject;
	friend class MyScene;
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	Sim_6NodedC0();
	~Sim_6NodedC0();

	//Simulation
	virtual void Initialize(const Sim_Generator_Output& configuration);
	virtual MPCG<SparseRowMatrix<Matrix3>>*  Solver() { return &m_Solver; }
	
	virtual bool GetIsStatic(uint idx) { return (idx < m_PhyxelIsStatic.size()) ? m_PhyxelIsStatic[idx] : false; }
	virtual void SetIsStatic(uint idx, bool is_static)
	{
		m_PhyxelIsStatic[idx] = is_static;
		UpdateConstraints();
	}

	virtual ProfilingTimer& GetTotalTimer() { return m_ProfilingTotalTime; }

	virtual int GetNumSubProfilers() { return Sim_6Noded_SubTimer_MAX; }
	virtual const ProfilingTimer& GetSubProfiler(int idx) { return m_ProfilingSubTimers[idx]; }


	//Intergratable
	virtual bool StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt);

	void UpdateConstraints();
	bool ValidateVelocityTimestep(const Vector3* pos_tmp);


	//Renderable
	virtual int GetNumTris() {
		return m_NumTriangles;
	}
	virtual void GetVertexWsPos(int triidx, const Vector3& gauss_point, const Vector3* positions, Vector3& out_pos);
	virtual void GetVertexRotation(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Matrix3& out_rotation);
	virtual void GetVertexStressStrain(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain);


	

protected:
	void SimpleCorotatedBuildAMatrix(float dt, const Vector3* positions, const Vector3* velocities);

	void InitGaussWeights();

	void BuildTransformationMatrix(const FETriangle& tri, const Vector3* pos, const Vec3& gaussPoint, const float warp_angle, Mat33& out_t, JaMatrix& out_ja, Mat26& out_dn);

	void CalcBMatrix(const FETriangle& tri, const Vector3* pos, const Vec3& gaussPoint, const float warp_angle, const VDisplacement& displacements, BMatrix& out_b, JaMatrix& out_ja, GMatrix& out_g);

protected:

	Eigen::Matrix<float, 12, 3> GaussPoint12;
	Eigen::Matrix<float, 12, 1> GaussWeight12;

	Mat33 E;
	float m_SumPreviousStrain;

	uint m_NumWidth, m_NumHeight;
	uint m_NumPhyxels, m_NumTriangles;
	float m_TotalArea;

	//Phyxel Data

	std::vector<Vector3>		m_PhyxelsPosInitial;
	std::vector<Vector3>		m_PhyxelForces;
	std::vector<Vector3>		m_PhyxelNormals;
	std::vector<Vector2>		m_PhyxelTexCoords;
	std::vector<bool>			m_PhyxelIsStatic;
	std::vector<float>			m_PhyxelsMass;

	std::vector<Mat33, Eigen::aligned_allocator<Mat33>>			m_TriMaterialDirections;

	//Structural Data
	std::vector<FETriangle> m_Triangles;
	std::vector<StiffnessMatrix, Eigen::aligned_allocator<StiffnessMatrix>> m_TriangleStiffness;


	MPCG<SparseRowMatrix<Matrix3>> m_Solver;	//Solver

	float angle = 0.0f;

	//Profiling
	ProfilingTimer	  m_ProfilingTotalTime;
	ProfilingTimer    m_ProfilingSubTimers[Sim_6Noded_SubTimer_MAX];
};