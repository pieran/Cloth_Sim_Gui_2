#pragma once

#include "Sim_Generator.h"

class Generator_Square_Grid : public Sim_Generator
{
public:
	Generator_Square_Grid();

	virtual void Generate(Sim_Generator_Output& out);

	void SetVisualSubdivisions(unsigned int visual_subdivisions) { SubDivisions = visual_subdivisions * 2 + 1; }
	unsigned int GetVisualSubdivisions() { return (SubDivisions - 1) / 2; }

	int GetVertIdx(int ix, int iy)
	{
		return SubDivisions * iy + ix;
	}
protected:
	unsigned int  GetNumTangents();
	unsigned int  GetNumVertices();

	void GenerateTriangles(std::vector<FETriangle>& triangles);
	void GeneratePositions(std::vector<FEVertDescriptor>& vert_descriptors, Vector3* vertices_transformed, Vector3* vertices_initial);
	void Generator_Square_Grid::GenerateTangents(Sim_Generator_Output& out, Vector3* tangents_transformed, Vector3* tangents_initial);
};

