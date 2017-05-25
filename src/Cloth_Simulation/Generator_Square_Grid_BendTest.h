#pragma once

#include "Generator_Square_Grid.h"

class Generator_Square_Grid_BendTest : public Generator_Square_Grid
{
public:
	Generator_Square_Grid_BendTest() : Generator_Square_Grid() {}

	virtual void Generate(Sim_Generator_Output& out) override
	{
		Generator_Square_Grid::Generate(out);

	
		uint mid = GetVertIdx((SubDivisions - 1) / 2, (SubDivisions - 1) / 2);

		//Set corners and middle to static
		out.Phyxel_Descriptors[GetVertIdx(0, 0)].isStatic = true;
		out.Phyxel_Descriptors[GetVertIdx(SubDivisions-1, 0)].isStatic = true;
		out.Phyxel_Descriptors[GetVertIdx(0, SubDivisions-1)].isStatic = true;
		out.Phyxel_Descriptors[GetVertIdx(SubDivisions-1, SubDivisions-1)].isStatic = true;
		out.Phyxel_Descriptors[mid].isStatic = true;

		//Create actuator to move mid point up 0.3m
		out.Actuators.clear();

		const float time_to_destination = 3.f;
		const Vector3 dir = Transform * Vector3(0, 0, -0.3f / time_to_destination);
		
		out.Actuators.push_back([=](float elapsedTime, Vector3* x, Vector3* dxdt)
		{
			uint mid = GetVertIdx((SubDivisions - 1) / 2, (SubDivisions - 1) / 2);
			if (elapsedTime < time_to_destination)
			{
				x[mid] = dir * elapsedTime;
				dxdt[mid] = dir;
			}
			else
			{
				dxdt[mid] = Vector3(0.f, 0.f, 0.f);
				//x[mid].y = 0.3f;
			}		
		});

	}
};