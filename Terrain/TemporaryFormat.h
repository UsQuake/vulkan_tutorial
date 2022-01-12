#pragma once
struct Model
{
	int vertex_count;
	int index_count;
	float *vertices;
	int *indices;

};

void ReadModel(const char *file_name, Model *ret_model);