#include "TemporaryFormat.h"
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <malloc.h>

void ReadModel(const char *file_name, Model *ret_model)
{
	FILE *file;
	file = fopen(file_name, "rb");
	
	fseek(file, 0, SEEK_END);
	int file_length = ftell(file);
	uint8_t *file_data;
	file_data = (uint8_t *)malloc(file_length);
	fseek(file, 0, SEEK_SET);
	fread(file_data, file_length, 1, file);
	fclose(file);

	int index = 0;
	memcpy(&ret_model->vertex_count, &file_data[index], 4);
	index += 4;



	ret_model->vertices = (float *)malloc(ret_model->vertex_count * 3 * 4);
	memcpy(ret_model->vertices, &file_data[index], ret_model->vertex_count * 3 * 4);
	index += ret_model->vertex_count * 3 * 4;

	memcpy(&ret_model->index_count, &file_data[index], 4);
	index += 4;


	ret_model->indices = (int *)malloc(ret_model->index_count * 4);
	memcpy(ret_model->indices, &file_data[index], ret_model->index_count * 4);
	index += ret_model->index_count * 4;

	free(file_data);
}