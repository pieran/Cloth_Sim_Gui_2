#pragma once
#include "SimulationDefines.h"
#include <functional>

struct Sim_Generator_Output
{
	void Release()
	{
		NumVertices = 0;
		NumTangents = 0;

		Triangles.clear();
		Phyxel_Descriptors.clear();

		Phyxels.clear();
		Phyxels_Initial.clear();

		Actuators.clear();
	}

	void CopyFrom(const Sim_Generator_Output& rhs)
	{
		if (this != &rhs)
		{
			Release();
			NumVertices = rhs.NumVertices;
			NumTangents = rhs.NumTangents;

			Triangles.resize(rhs.Triangles.size());
			Phyxel_Descriptors.resize(rhs.Phyxel_Descriptors.size());
			Phyxels.resize(rhs.Phyxels.size());
			Phyxels_Initial.resize(rhs.Phyxels_Initial.size());
			Actuators.resize(rhs.Actuators.size());

			memcpy(&Triangles[0], &rhs.Triangles[0], rhs.Triangles.size() * sizeof(FETriangle));
			memcpy(&Phyxel_Descriptors[0], &rhs.Phyxel_Descriptors[0], rhs.Phyxel_Descriptors.size() * sizeof(FEVertDescriptor));
			memcpy(&Phyxels[0], &rhs.Phyxels[0], rhs.Phyxels.size() * sizeof(Vector3));
			memcpy(&Phyxels_Initial[0], &rhs.Phyxels_Initial[0], rhs.Phyxels_Initial.size() * sizeof(Vector3));

			for (int i = 0; i < rhs.Actuators.size(); ++i)
				Actuators[i] = rhs.Actuators[i];
		}
	}
	uint NumVertices, NumTangents;

	std::vector<FETriangle> Triangles;
	std::vector<FEVertDescriptor> Phyxel_Descriptors;
	std::vector<Vector3> Phyxels;
	std::vector<Vector3> Phyxels_Initial;

	std::vector<std::function<void(float elapsed, Vector3* x, Vector3* dxdt)>> Actuators;
};

class Sim_Generator
{
public:
	virtual void Generate(Sim_Generator_Output& out) = 0;

	Matrix4 Transform;
	unsigned int SubDivisions;
};
