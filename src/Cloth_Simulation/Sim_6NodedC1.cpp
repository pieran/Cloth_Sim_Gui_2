#include "Sim_6NodedC1.h"
#include <glcore\NCLDebug.h>
#include "utils.h"

Sim_6NodedC1::Sim_6NodedC1() : Sim_Rendererable()
{
	m_ProfilingTotalTime.SetAlias("Total Time");
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Rotations].SetAlias("Geb Rotation");
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_BuildMatrices].SetAlias("Gen Matrix");
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Solver].SetAlias("Solver");

	InitGaussWeights();

	const float Y = 2500.0f;	//Youngs Modulus
	const float v = 0.3f;		//Poisson coefficient
	E.setZero();
	E(0, 0) = 1.0f;
	E(1, 0) = v;
	E(0, 1) = v;
	E(1, 1) = 1.0f;
	E(2, 2) = (1.0f - v) * 0.5f;
	E *= Y / (1.0f - v * v);
}

Sim_6NodedC1::~Sim_6NodedC1()
{
}

void Sim_6NodedC1::Initialize(const Sim_Generator_Output& configuration)
{


	//Gen Vertices
	m_NumTotal = configuration.Phyxels.size();
	m_NumPhyxels = configuration.NumVertices;
	m_NumTangents = configuration.NumTangents;

	m_Solver.AllocateMemory(m_NumPhyxels + m_NumTangents);
	m_Solver.m_A.resize(m_NumPhyxels + m_NumTangents);


	m_PhyxelsPosInitial.resize(m_NumPhyxels);
	m_PhyxelForces.resize(m_NumPhyxels);

	m_PhyxelIsStatic.resize(m_NumTotal);
	m_PhyxelsMass.resize(m_NumPhyxels);
	m_PhyxelTexCoords.resize(m_NumPhyxels);

	m_PhyxelsTangentInitial.resize(m_NumTangents);
	memcpy(&m_PhyxelsPosInitial[0], &configuration.Phyxels_Initial[0], m_NumPhyxels * sizeof(Vector3));
	memcpy(&m_PhyxelsTangentInitial[0], &configuration.Phyxels_Initial[m_NumPhyxels], m_NumTangents * sizeof(Vector3));
	
	for (unsigned int i = 0; i < m_NumPhyxels; ++i)
	{
		const FEVertDescriptor& v = configuration.Phyxel_Descriptors[i];

		m_PhyxelIsStatic[i] = v.isStatic;

		if (!m_PhyxelIsStatic[i])
		{
			m_PhyxelForces[i] = Vector3(0.f, 0.f, 0.f);// GRAVITY / float(m_NumPhyxels);
			//m_PhyxelForces[i] = GRAVITY / float(m_NumPhyxels);
		}

		m_PhyxelTexCoords[i] = v.tCoord;
	}

	for (unsigned int i = 0; i < m_NumTangents; ++i)
		m_PhyxelIsStatic[i + m_NumPhyxels] = false;



	//Populate Triangles & Add Additional Mid-Points
	m_NumTriangles = configuration.Triangles.size();
	m_Triangles.resize(m_NumTriangles);

	float totalArea = 0.0f;
	for (unsigned int i = 0; i < m_NumTriangles; ++i)
	{
		unsigned int i3 = i * 3;
		memcpy(m_Triangles[i].phyxels, configuration.Triangles[i].phyxels, sizeof(FETriangle));
	
		//Simple area estimation (ONLY WORKS IF TRIANGLE HAS STRAIGHT EDGES AT REST!!!!)
		Vector3 a = m_PhyxelsPosInitial[m_Triangles[i].v1];
		Vector3 b = m_PhyxelsPosInitial[m_Triangles[i].v2];
		Vector3 c = m_PhyxelsPosInitial[m_Triangles[i].v3];

		Vector3 e1 = a - c;
		Vector3 e2 = b - c;

		totalArea += 0.5f * abs(e1.x * e2.y - e1.y * e2.x);
	}



	float uniform_mass = (totalArea * mass_density) / m_NumPhyxels;
	for (unsigned int i = 0; i < m_NumPhyxels; ++i)
	{
		m_PhyxelsMass[i] = uniform_mass;
	}

	//Build Global Matricies (To be deleted at the start of the simulation)
	SimpleCorotatedBuildAMatrix(0.0f, &configuration.Phyxels[0], NULL);
	m_Solver.m_A.zero_memory();


	m_Solver.ResetMemory();
	m_Solver.m_A.zero_memory();
	UpdateConstraints();
}

void  Sim_6NodedC1::UpdateConstraints()
{
#pragma omp parallel for
	for (int i = 0; i < (int)m_NumTotal; ++i)
	{
		if (m_PhyxelIsStatic[i])
		{
			m_Solver.m_Constraints[i] = Matrix3::ZeroMatrix;
		}
		else
		{
			m_Solver.m_Constraints[i] = Matrix3::Identity;
		}
	}
}

bool Sim_6NodedC1::StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt)
{
	m_ProfilingTotalTime.BeginTiming();
	for (int i = 0; i < Sim_6NodedC1_SubTimer_MAX; ++i)
	{
		m_ProfilingSubTimers[i].ResetTotalMs();
	}
	m_Solver.ResetProfilingData();

	Vector3 sub_grav = gravity / m_NumPhyxels;
#pragma omp parallel for
	for (int i = 0; i < (int)m_NumPhyxels; ++i)
	{
		m_PhyxelForces[i] = sub_grav;
	}

	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Rotations].BeginTiming();
	//	BuildRotationAndNormals();
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Rotations].EndTimingAdditive();

	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Solver].BeginTiming();
	m_Solver.ResetMemory();
	m_Solver.m_A.zero_memory();
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Solver].EndTimingAdditive();

	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_BuildMatrices].BeginTiming();
	SimpleCorotatedBuildAMatrix(dt, in_x, in_dxdt);
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_BuildMatrices].EndTimingAdditive();

	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Solver].BeginTiming();
	//m_Solver.SolveWithPreviousResult();
	m_Solver.SolveWithGuess(in_dxdt);
	m_ProfilingSubTimers[Sim_6NodedC1_SubTimer_Solver].EndTimingAdditive();

	m_ProfilingTotalTime.EndTimingAdditive();

	memcpy(out_dxdt, &m_Solver.m_X[0], m_NumTotal * sizeof(Vector3));
	return true;
}


bool Sim_6NodedC1::ValidateVelocityTimestep()
{
	//Check whether any of the edges of the triangles change by more than 10%
	/*for (uint i = 0; i < m_NumTriangles; ++i)
	{
		uint idxA = m_Triangles[i].phyxels[0];
		uint idxB = m_Triangles[i].phyxels[1];
		uint idxC = m_Triangles[i].phyxels[2];

		Vector3 ABnew = m_PhyxelsPosTemp[idxB] - m_PhyxelsPosTemp[idxA];
		Vector3 ACnew = m_PhyxelsPosTemp[idxC] - m_PhyxelsPosTemp[idxA];
		Vector3 BCnew = m_PhyxelsPosTemp[idxC] - m_PhyxelsPosTemp[idxB];

		Vector3 ABold = m_PhyxelsPos[idxB] - m_PhyxelsPos[idxA];
		Vector3 ACold = m_PhyxelsPos[idxC] - m_PhyxelsPos[idxA];
		Vector3 BCold = m_PhyxelsPos[idxC] - m_PhyxelsPos[idxB];

		float ABLenDif = 1.0f - ABnew.Length() / ABold.Length();
		float ACLenDif = 1.0f - ACnew.Length() / ACold.Length();
		float BCLenDif = 1.0f - BCnew.Length() / BCold.Length();

		const float tolerence = 0.1f;
		if (fabs(ABLenDif) > tolerence) return false;
		if (fabs(ACLenDif) > tolerence) return false;
		if (fabs(BCLenDif) > tolerence) return false;
	}*/

	return true;
}

void Sim_6NodedC1::InitGaussWeights()
{

	//Define Gauss points for numerical intergration
	GaussPoint12(0, 0) = 0.873821971016996;
	GaussPoint12(0, 1) = 0.063089014491502;
	GaussPoint12(0, 2) = 0.063089014491502;
	GaussPoint12(1, 0) = 0.063089014491502;
	GaussPoint12(1, 1) = 0.063089014491502;
	GaussPoint12(1, 2) = 0.873821971016996;
	GaussPoint12(2, 0) = 0.063089014491502;
	GaussPoint12(2, 1) = 0.873821971016996;
	GaussPoint12(2, 2) = 0.063089014491502;

	for (int i = 0; i < 3; ++i)
		GaussWeight12(i) = 0.050844906370207;

	GaussPoint12(3, 0) = 0.501426509658179;
	GaussPoint12(3, 1) = 0.249286745170910;
	GaussPoint12(3, 2) = 0.249286745170910;
	GaussPoint12(4, 0) = 0.249286745170910;
	GaussPoint12(4, 1) = 0.249286745170910;
	GaussPoint12(4, 2) = 0.501426509658179;
	GaussPoint12(5, 0) = 0.249286745170910;
	GaussPoint12(5, 1) = 0.501426509658179;
	GaussPoint12(5, 2) = 0.249286745170910;

	for (int i = 3; i < 6; ++i)
		GaussWeight12(i) = 0.116786275726379;


	GaussPoint12(6, 0) = 0.636502499121399;
	GaussPoint12(6, 1) = 0.310352451033784;
	GaussPoint12(6, 2) = 0.053145049844817;
	GaussPoint12(7, 0) = 0.636502499121399;
	GaussPoint12(7, 1) = 0.053145049844817;
	GaussPoint12(7, 2) = 0.310352451033784;
	GaussPoint12(8, 0) = 0.310352451033784;
	GaussPoint12(8, 1) = 0.636502499121399;
	GaussPoint12(8, 2) = 0.053145049844817;

	GaussPoint12(9, 0) = 0.310352451033784;
	GaussPoint12(9, 1) = 0.053145049844817;
	GaussPoint12(9, 2) = 0.636502499121399;
	GaussPoint12(10, 0) = 0.053145049844817;
	GaussPoint12(10, 1) = 0.310352451033784;
	GaussPoint12(10, 2) = 0.636502499121399;
	GaussPoint12(11, 0) = 0.053145049844817;
	GaussPoint12(11, 1) = 0.636502499121399;
	GaussPoint12(11, 2) = 0.310352451033784;

	for (int i = 6; i < 12; ++i)
		GaussWeight12(i) = 0.082851075618374;

}

void Sim_6NodedC1::CalcBMatrix(const FETriangle& tri, const Vector3* pos, const Vector3* tans, const Vec3& gaussPoint, const float warp_angle, const Eigen::Matrix<float, 45, 1>& displacements, BMatrix_C1& out_b, JaMatrix& out_ja, Eigen::Matrix<float, 6, 45>& out_g)
{
	BaseMatrix DN, a;
	Mat33 T;
	BMatrix_C1 B_l, B_nl;
	Eigen::Matrix<float, 3, 6> derivatives;
	Eigen::Matrix<float, 6, 1> delta;

	//BuildTransformationMatrix(tri, pos, gaussPoint, warp_angle, T, out_ja, DN);
	CalcRotationC1(tri, pos, tans, Vector3(gaussPoint.x(), gaussPoint.y(), gaussPoint.z()), warp_angle, T, out_ja, DN);

	Mat22 Ja_inv = out_ja.inverse();

	a = Ja_inv * DN;

	for (int i = 0; i < 15; ++i)
	{
		B_l(0, i * 3) = T(0, 0) * a(0, i);
		B_l(0, i * 3 + 1) = T(0, 1) * a(0, i);
		B_l(0, i * 3 + 2) = T(0, 2) * a(0, i);

		B_l(1, i * 3) = T(1, 0) * a(1, i);
		B_l(1, i * 3 + 1) = T(1, 1) * a(1, i);
		B_l(1, i * 3 + 2) = T(1, 2) * a(1, i);

		B_l(2, i * 3) = T(0, 0) * a(1, i) + T(1, 0) * a(0, i);
		B_l(2, i * 3 + 1) = T(0, 1) * a(1, i) + T(1, 1) * a(0, i);
		B_l(2, i * 3 + 2) = T(0, 2) * a(1, i) + T(1, 2) * a(0, i);
	}

	//Build G Matrix
	for (int n = 0; n < 15; ++n)
	{
		for (int i = 0; i < 3; ++i)
		{
			out_g(i, n * 3) = T(i, 0) * a(0, n);
			out_g(i, n * 3 + 1) = T(i, 1) * a(0, n);
			out_g(i, n * 3 + 2) = T(i, 2) * a(0, n);
		}

		for (int i = 3; i < 6; ++i)
		{
			out_g(i, n * 3) = T(i - 3, 0) * a(1, n);
			out_g(i, n * 3 + 1) = T(i - 3, 1) * a(1, n);
			out_g(i, n * 3 + 2) = T(i - 3, 2) * a(1, n);
		}
	}

	//Find displacement dependant terms of local Bmatrix
	delta = out_g * displacements;

	derivatives.setZero();
	derivatives(0, 0) = delta[0];
	derivatives(0, 1) = delta[1];
	derivatives(0, 2) = delta[2];
	derivatives(1, 3) = delta[3];
	derivatives(1, 4) = delta[4];
	derivatives(1, 5) = delta[5];

	derivatives(2, 0) = delta[3];
	derivatives(2, 1) = delta[4];
	derivatives(2, 2) = delta[5];
	derivatives(2, 3) = delta[0];
	derivatives(2, 4) = delta[1];
	derivatives(2, 5) = delta[2];

	B_nl = 0.5 * derivatives * out_g;

	out_b = B_l + B_nl;
}

void Sim_6NodedC1::SimpleCorotatedBuildAMatrix(float dt, const Vector3* positions, const Vector3* velocities)
{
	const float dt2 = dt * dt;
	const float dt2_viscos = dt2 + V_SCALAR * dt;

	const float mass_dampening = 1.f + 1E-6f;
	const float mass_dampening_tangents = 1.f + 1E-3f;
	if (velocities == NULL)
	{
#pragma omp parallel for
		for (int i = 0; i < (int)m_NumPhyxels; ++i)
		{
			m_Solver.m_A(i, i) = Matrix3::Identity * m_PhyxelsMass[i] * mass_dampening;
			m_Solver.m_B[i] = m_PhyxelForces[i] * dt;
		}
	}
	else
	{
#pragma omp parallel for
		for (int i = 0; i < (int)m_NumPhyxels; ++i)
		{
			m_Solver.m_A(i, i) = Matrix3::Identity * m_PhyxelsMass[i] * mass_dampening;
			m_Solver.m_B[i] = velocities[i] * m_PhyxelsMass[i] + m_PhyxelForces[i] * dt;
		}

		float tangentMass =  0.0002f;
		for (int i = 0; i < (int)m_NumTangents; ++i)
		{
			m_Solver.m_A(m_NumPhyxels + i, m_NumPhyxels + i) = Matrix3::Identity * tangentMass * mass_dampening_tangents;
			m_Solver.m_B[m_NumPhyxels + i] = velocities[m_NumPhyxels + i] * tangentMass;
		}
	}


	Matrix3 RK, RKR, ReT;
	Vector3 posAB, posIAB;




	BMatrix_C1 B_nl, B_0;
	JaMatrix Ja;
	Eigen::Matrix<float, 6, 45> G;

	Eigen::Matrix<float, 45, 1> d_g, d_0, force;
	d_0.setZero();

	Eigen::Matrix<float, 45, 45> K_E, K_S, K_T;
	Eigen::Matrix<float, 6, 6> M; M.setZero();
	
	for (uint i = 0; i < m_NumTriangles; ++i)
	{
		FETriangle& tri = m_Triangles[i];

		K_E.setZero();
		K_S.setZero();

		for (int j = 0; j < 6; ++j)
		{
			Vector3 pos = (positions[tri.phyxels[j]] - m_PhyxelsPosInitial[tri.phyxels[j]]);

			d_g(j * 3 + 0, 0) = pos.x; d_g(j * 3 + 1, 0) = pos.y; d_g(j * 3 + 2, 0) = pos.z;
		}

		for (int j = 0; j < 9; ++j)
		{
			Vector3 tan = (positions[m_NumPhyxels + tri.tangents[j]] - m_PhyxelsTangentInitial[tri.tangents[j]]);// *tri.tan_multipliers[j];

			d_g(j * 3 + 18, 0) = tan.x; d_g(j * 3 + 19, 0) = tan.y; d_g(j * 3 + 20, 0) = tan.z;
		}

		 force.setZero();
		for (uint j = 0; j < 12; ++j)
		{
			CalcBMatrix(tri, &m_PhyxelsPosInitial[0], &m_PhyxelsTangentInitial[0], GaussPoint12.row(j), 0.0f, d_g, B_nl, Ja, G);

			Vec3 strain = B_nl * d_g;
			Vec3 stress = E * strain;

			CalcBMatrix(tri, positions, &positions[m_NumPhyxels], GaussPoint12.row(j), 0.0f, d_0, B_0, Ja, G);
		
			float area = Ja.determinant() * 0.5f;
			if (area < 0)
			{
				printf("ERROR:: Element %d:%d has a negative area!!!\n", i, j);
			}
			
			float tfactor = GaussWeight12(j) * area;


			
			M(0, 0) = stress.x();
			M(1, 1) = stress.x();
			M(2, 2) = stress.x();

			M(0, 3) = stress.z();
			M(1, 4) = stress.z();
			M(2, 5) = stress.z();
			M(3, 0) = stress.z();
			M(4, 1) = stress.z();
			M(5, 2) = stress.z();

			M(3, 3) = stress.y();
			M(4, 4) = stress.y();
			M(5, 5) = stress.y();



			K_E += B_0.transpose() * E * B_nl * tfactor;
			K_S += G.transpose() * M * G * tfactor;

			force += B_0.transpose() * stress * tfactor;
		}

		K_T = K_E + K_S;


		//Convert Eigen Matrices back to global A Matrix + B Vectors for global solver
		for (uint j = 0; j < 15; ++j)
		{
			uint idxJ = (j < 6) ? tri.phyxels[j] : m_NumPhyxels + tri.tangents[j-6];
			if (j < 6)
				m_Solver.m_B[idxJ] -= Vector3(force(j * 3), force(j * 3 + 1), force(j * 3 + 2)) * dt;
			if (j >= 6)
				m_Solver.m_B[idxJ] -= Vector3(force(j * 3), force(j * 3 + 1), force(j * 3 + 2)) * dt;// *tri.tan_multipliers[j - 6];

		//	m_PhyxelsPos[tri.phyxels[j]] += Vector3(force(j * 3), force(j * 3 + 1), force(j * 3 + 2)) * dt * 0.001f;

			for (uint k = 0; k < 15; ++k)
			{
				uint idxK = (k < 6) ? tri.phyxels[k] : m_NumPhyxels + tri.tangents[k-6];
				Matrix3 submtx;

				submtx._11 = K_T(j * 3 + 0, k * 3 + 0);
				submtx._12 = K_T(j * 3 + 0, k * 3 + 1);
				submtx._13 = K_T(j * 3 + 0, k * 3 + 2);
				submtx._21 = K_T(j * 3 + 1, k * 3 + 0);
				submtx._22 = K_T(j * 3 + 1, k * 3 + 1);
				submtx._23 = K_T(j * 3 + 1, k * 3 + 2);
				submtx._31 = K_T(j * 3 + 2, k * 3 + 0);
				submtx._32 = K_T(j * 3 + 2, k * 3 + 1);
				submtx._33 = K_T(j * 3 + 2, k * 3 + 2);

				m_Solver.m_A(idxJ, idxK) += submtx * dt * dt;
			}
		}


	}
}

void Sim_6NodedC1::BuildNaturalCoordinateBasisVectors(const FETriangle& tri, const Vector3& gaussPoint, BaseMatrix& out_dn)
{
	Vector3 gp = gaussPoint;
	Vector3 gp2 = gp * gp;
	Vector3 gp3 = gp * gp * gp;
	Vector3 gp4 = gp2 * gp2;
	Vector3 gp5 = gp3 * gp2;

	float gxy = gp.x * gp.y;
	float gx2y = gp2.x * gp.y;
	float gxy2 = gp.x * gp2.y;
	float gx3y = gp3.x * gp.y;
	float gx2y2 = gp2.x * gp2.y;
	float gxy3 = gp.x * gp3.y;

	//Build natural coordinate basis vectors
	float coeffs_x[]
	{
		-10 * gp.x + 42 * gp2.x - 32 * gp3.x + 6 * (2 * gxy - 3 * gx2y - 2 * gxy2),
		6 * (gp2.y - gp3.y - 2 * gxy2),
		0,
		-16 * gp.y + 48 * (2 * gxy + gp2.y) - 32 * (3 * gx2y + gp3.y) - 96 * gxy2,
		0,
		-16 * gp.z + 48 * (2 * gp.x*gp.z + gp2.z) - 32 * (gp3.z + 3 * gp.z*gp2.x) - 96 * gp.x*gp2.z,

		0.5 * (-gp.y + 2 * gxy + 3 * gp2.y) + 3 * gx2y - 4 * gxy2 - gp3.y,
		0.5 * (-gp.z + 2 * gp.x*gp.z + 3 * gp2.z) + 3 * gp2.x*gp.z - 4 * gp.x*gp2.z - gp3.z,
		0.5 * (-gp.y + 6 * gxy + gp2.y) - 3 * gx2y - 4 * gxy2 + gp3.y,
		0,
		0.5 * (gp.z - 6 * gp.x*gp.z - gp2.z) + 3 * gp2.x*gp.z - gp3.z + 4 * gp.x*gp2.z,
		0,

		-4 * gp.y + 12 * (2 * gxy + gp2.y) - 8 * (3 * gx2y + 4 * gxy2 + gp3.y),
		0,
		-4 * gp.z + 12 * (2 * gp.x * gp.z + gp2.z) - 8 * (3 * gp2.x * gp.z + 4 * gp.x * gp2.z + gp3.z)
	};
	float coeffs_y[]
	{
		6 * (gp2.x - gp3.x - 2 * gx2y),
		-10 * gp.y + 42 * gp2.y - 32 * gp3.y + 6 * (2 * gxy - 3 * gxy2 - 2 * gx2y),
		6 * (gp2.z - gp3.z - 2 * gp.y*gp2.z),
		-16 * gp.x + 48 * (gp2.x + 2 * gxy) - 32 * (gp3.x + 3 * gxy2) - 96 * gx2y,
		-16 * gp.z + 48 * (2 * gp.y*gp.z + gp2.z) - 32 * (gp3.z + 3 * gp.z*gp2.y) - 96 * gp.y*gp2.z,
		0,

		0.5 * (-gp.x + gp2.x + 6 * gxy) + gp3.x - 4 * gx2y - 3 * gxy2,
		0,
		0.5 * (-gp.x + 3 * gp2.x + 2 * gxy) - gp3.x - 4 * gx2y + 3 * gxy2,
		0.5 * (-gp.z + 2 * gp.y*gp.z + 3 * gp2.z) + 3 * gp2.y*gp.z - 4 * gp.y*gp2.z - gp3.z,
		0,
		0.5 * (gp.z - 6 * gp.y*gp.z - gp2.z) + 3 * gp2.y*gp.z - gp3.z + 4 * gp.y*gp2.z,

		-4 * gp.x + 12 * (gp2.x + 2 * gxy) - 8 * (gp3.x + 4 * gx2y + 3 * gxy2),
		-4 * gp.z + 12 * (2 * gp.y * gp.z + gp2.z) - 8 * (3 * gp2.y * gp.z + 4 * gp.y * gp2.z + gp3.z),
		0
	};
	float coeffs_z[]
	{
		0,
		0,
		-10 * gp.z + 42 * gp2.z - 32 * gp3.z + 6 * (2 * gp.y*gp.z - 3 * gp.y*gp2.z - 2 * gp2.y*gp.z),
		0,
		-16 * gp.y + 48 * (gp2.y + 2 * gp.z * gp.y) - 32 * (3 * gp2.z*gp.y + gp3.y) - 96 * gp2.y*gp.z,
		-16 * gp.x + 48 * (gp2.x + 2 * gp.z * gp.x) - 32 * (3 * gp2.z*gp.x + gp3.x) - 96 * gp2.x*gp.z,

		0,
		0.5 * (-gp.x + gp2.x + 6 * gp.x*gp.z) + gp3.x - 4 * gp2.x*gp.z - 3 * gp.x*gp2.z,
		0,
		0.5 * (-gp.y + gp2.y + 6 * gp.y*gp.z) + gp3.y - 4 * gp2.y*gp.z - 3 * gp.y*gp2.z,
		0.5 * (gp.x - 3 * gp2.x - 2 * gp.x*gp.z) + gp3.x - 3 * gp.x*gp2.z + 4 * gp2.x*gp.z,
		0.5 * (gp.y - 3 * gp2.y - 2 * gp.y*gp.z) + gp3.y - 3 * gp.y*gp2.z + 4 * gp2.y*gp.z,

		0,
		-4 * gp.y + 12 * (gp2.y + 2 * gp.y * gp.z) - 8 * (gp3.y + 4 * gp2.y * gp.z + 3 * gp.y * gp2.z),
		-4 * gp.x + 12 * (gp2.x + 2 * gp.x * gp.z) - 8 * (gp3.x + 4 * gp2.x * gp.z + 3 * gp.x * gp2.z)
	};

	for (int i = 0; i < 6; ++i)
	{
		out_dn(0, i) = coeffs_x[i] - coeffs_z[i];
		out_dn(1, i) = coeffs_y[i] - coeffs_z[i];
	}

	for (int i = 0; i < 9; ++i)
	{
		out_dn(0, 6 + i) = (coeffs_x[6 + i] - coeffs_z[6 + i]) *tri.tan_multipliers[i];
		out_dn(1, 6 + i) = (coeffs_y[6 + i] - coeffs_z[6 + i]) *tri.tan_multipliers[i];
	}
}

void Sim_6NodedC1::CalcRotationC1(const FETriangle& tri, const Vector3* pos, const Vector3* tans, const Vector3& gaussPoint, float warp_angle, Eigen::Matrix3f& out_rot, JaMatrix& out_ja, BaseMatrix& out_dn)
{
	BuildNaturalCoordinateBasisVectors(tri, gaussPoint, out_dn);
	
	//Build local fill direction (local warp direction)
	Vector3 vFill(-sin(warp_angle), cos(warp_angle), 0.0f);

	//Compute pure deformation in XZ and YZ axis
	Vector3 dir_xz, dir_yz;

	for (int i = 0; i < 6; ++i)
	{
		dir_xz += pos[tri.phyxels[i]] * out_dn(0, i);
		dir_yz += pos[tri.phyxels[i]] * out_dn(1, i);
	}
	for (int i = 0; i < 9; ++i)
	{
		dir_xz += tans[tri.tangents[i]] * out_dn(0, 6 + i);
		dir_yz += tans[tri.tangents[i]] * out_dn(1, 6 + i);
	}

	//Build the Rotation Matrix
	Vector3 V_z = Vector3::Cross(dir_xz, dir_yz);  V_z.Normalise();
	//if (V_z.z < 0.0f)
	//{
	//	V_z = -V_z;
	//}

	Vector3 V_tx = Vector3::Cross(dir_yz, V_z);
	Vector3 V_ty = Vector3::Cross(dir_xz, V_z);

	Vector3 avg = (V_tx + V_ty) * 0.5;

	Vector3 V_x = Vector3::Cross(dir_yz, V_z);
	V_x.Normalise();

	Vector3 V_y = Vector3::Cross(V_z, V_x);
	V_y.Normalise();

	//Vector3 V_x = Vector3::Cross(dir_yz, V_z); V_x.Normalise();
	//Vector3 V_y = Vector3::Cross(V_z, V_x); V_y.Normalise();

	out_rot(0, 0) = V_x.x; out_rot(0, 1) = V_x.y; out_rot(0, 2) = V_x.z;
	out_rot(1, 0) = V_y.x; out_rot(1, 1) = V_y.y; out_rot(1, 2) = V_y.z;
	out_rot(2, 0) = V_z.x; out_rot(2, 1) = V_z.y; out_rot(2, 2) = V_z.z;


	//	//Build the Jacobian Matrix
	out_ja(0, 0) = Vector3::Dot(dir_xz, V_x);
	out_ja(0, 1) = Vector3::Dot(dir_xz, V_y);
	out_ja(1, 0) = Vector3::Dot(dir_yz, V_x);
	out_ja(1, 1) = Vector3::Dot(dir_yz, V_y);
}

void Sim_6NodedC1::GetVertexWsPos(int triidx, const Vector3& gp, const Vector3* positions, Vector3& out_pos)
{
	const auto& t = m_Triangles[triidx];

	Vector3 gp2 = gp * gp;
	Vector3 gp3 = gp * gp * gp;
	Vector3 gp4 = gp2 * gp2;
	Vector3 gp5 = gp3 * gp2;

	float gxy = gp.x * gp.y;
	float gx2y = gp2.x * gp.y;
	float gxy2 = gp.x * gp2.y;
	float gx3y = gp3.x * gp.y;
	float gx2y2 = gp2.x * gp2.y;
	float gxy3 = gp.x * gp3.y;

	float FormFunction[]{
			-5 * gp2.x + 14 * gp3.x - 8 * gp4.x + 6 * (gx2y - gx3y - gx2y2),												//PA
			-5 * gp2.y + 14 * gp3.y - 8 * gp4.y + 6 * (gxy2 - gxy3 - gx2y2),												//PB
			-5 * gp2.z + 14 * gp3.z - 8 * gp4.z + 6 * (gp.y*gp2.z - gp.y*gp3.z - gp2.y*gp2.z),								//PC - Note: Y here is mute and can be replaced by 'X'
			-16 * gxy + 48 * (gx2y + gxy2) - 32 * (gx3y + gxy3) - 48 * gx2y2,												//PAB
			-16 * gp.y * gp.z + 48 * (gp2.y*gp.z + gp2.z * gp.y) - 32 * (gp3.z*gp.y + gp.z*gp3.y) - 48 * gp2.y*gp2.z,		//PBC
			-16 * gp.x * gp.z + 48 * (gp2.x*gp.z + gp2.z * gp.x) - 32 * (gp3.z*gp.x + gp.z*gp3.x) - 48 * gp2.x*gp2.z,		//PAC


			0.5 * (-gxy + gx2y + 3 * gxy2) + gx3y - 2 * gx2y2 - gxy3,														//T12
			0.5 * (-gp.x*gp.z + gp2.x*gp.z + 3 * gp.x*gp2.z) + gp3.x*gp.z - 2 * gp2.x*gp2.z - gp.x*gp3.z,					//T13

			0.5 * (-gxy + 3 * gx2y + gxy2) - gx3y - 2 * gx2y2 + gxy3,														//T21
			0.5 * (-gp.y*gp.z + gp2.y*gp.z + 3 * gp.y*gp2.z) + gp3.y*gp.z - 2 * gp2.y*gp2.z - gp.y*gp3.z,					//T23

			0.5 * (gp.x*gp.z - 3 * gp2.x*gp.z - gp.x*gp2.z) + gp3.x*gp.z - gp.x*gp3.z + 2 * gp2.x*gp2.z,					//T31
			0.5 * (gp.y*gp.z - 3 * gp2.y*gp.z - gp.y*gp2.z) + gp3.y*gp.z - gp.y*gp3.z + 2 * gp2.y*gp2.z,					//T32

			-4 * gxy + 12 * (gx2y + gxy2) - 8 * (gx3y + 2 * gx2y2 + gxy3),													//TAB
			-4 * gp.y * gp.z + 12 * (gp2.y * gp.z + gp.y * gp2.z) - 8 * (gp3.y * gp.z + 2 * gp2.y * gp2.z + gp.y * gp3.z),	//TBC
			-4 * gp.x * gp.z + 12 * (gp2.x * gp.z + gp.x * gp2.z) - 8 * (gp3.x * gp.z + 2 * gp2.x * gp2.z + gp.x * gp3.z),	//TAC
	};

	Vector3 ws_pos = Vector3(0.f, 0.f, 0.f);
	for (int i = 0; i < 6; ++i)
	{
		ws_pos += positions[t.phyxels[i]] * FormFunction[i];
	}
	for (int i = 0; i < 9; ++i)
	{
		ws_pos += positions[m_NumPhyxels + t.tangents[i]] * (FormFunction[6 + i] * t.tan_multipliers[i]);
	}

	out_pos = ws_pos;
}

void Sim_6NodedC1::GetVertexRotation(int triidx, const Vector3& gp, const Vector3* positions, const Vector3& wspos, Matrix3& out_rotation)
{
	BaseMatrix Dn;
	Eigen::Matrix3f rot;
	JaMatrix Ja;
	const auto& t = m_Triangles[triidx];
	CalcRotationC1(t, positions, &positions[m_NumPhyxels], gp, 0.f, rot, Ja, Dn);

	memcpy(&out_rotation._11, &rot, sizeof(Matrix3));
}

void Sim_6NodedC1::GetVertexStressStrain(int triidx, const Vector3& gp, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain)
{
	const auto& t = m_Triangles[triidx];

	Eigen::Matrix<float, 45, 1> d_g;
	BMatrix_C1 B_nl;
	JaMatrix Ja;
	Eigen::Matrix<float, 6, 45> G;

	for (int j = 0; j < 6; ++j)
	{
		Vector3 pos = (positions[t.phyxels[j]] - m_PhyxelsPosInitial[t.phyxels[j]]);

		d_g(j * 3 + 0, 0) = pos.x; d_g(j * 3 + 1, 0) = pos.y; d_g(j * 3 + 2, 0) = pos.z;
	}

	for (int j = 0; j < 9; ++j)
	{
		Vector3 tan = (positions[m_NumPhyxels + t.tangents[j]] - m_PhyxelsTangentInitial[t.tangents[j]]);// *t.tan_multipliers[j];

		d_g(j * 3 + 18, 0) = tan.x; d_g(j * 3 + 19, 0) = tan.y; d_g(j * 3 + 20, 0) = tan.z;
	}

	CalcBMatrix(t, &m_PhyxelsPosInitial[0], &m_PhyxelsTangentInitial[0], Vec3(gp.x, gp.y, gp.z), 0.0f, d_g, B_nl, Ja, G);

	Vec3 strain = B_nl * d_g;
	Vec3 stress = E * strain;

	memcpy(&out_stress, &stress, sizeof(Vector3));
	memcpy(&out_strain, &strain, sizeof(Vector3));
}

