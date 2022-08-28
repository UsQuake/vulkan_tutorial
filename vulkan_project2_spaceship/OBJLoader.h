#pragma once
#include <iostream>
#include <fstream>
#include "MemoryPools.h"
using namespace std;



#define MAX_STRING_CHAR_COUNT 100
#define MAX_TOKEN_CHAR_COUNT 40

const glm::vec3 Sub(glm::vec3 V1, glm::vec3 V2) {
	return glm::vec3(V1.x - V2.x, V1.y - V2.y, V1.z - V2.z);
}
const glm::vec3 GammaToLinear(const glm::vec3& color)
{
	return  glm::vec3(color.x * color.x, color.y * color.y, color.z * color.z);
}
char** returnSplit(const char* line, char seperator, unsigned int* return_count) {
	int line_index = 0;

	int token_index = 0;
	int _count = 0;
	while (true) {
		char chr = line[line_index];
		if (chr == seperator) _count++;
		else if (chr == '\0') {
			_count++;
			break;
		}
		line_index++;
	}
	line_index = 0;
	char** chrResult = new char* [_count];

	int tmp_index = 0;
	char tmp_token[MAX_TOKEN_CHAR_COUNT] = { 0 };

	while (token_index != _count) {

		char chr = line[line_index];

		if (chr == seperator || chr == '\0') {
			//Break;
			tmp_token[tmp_index] = '\0';

			tmp_index = 0;

			char* ind = new char[MAX_TOKEN_CHAR_COUNT];
			strcpy_s(ind, MAX_TOKEN_CHAR_COUNT, tmp_token);
			chrResult[token_index] = ind;


			token_index++;

		}
		else {
			tmp_token[tmp_index] = chr;
			tmp_index++;
		}
		//chr 적용
		line_index++;
	}
	*return_count = _count;
	return chrResult;
}


void ReadOBJ(const char* file_name, VertexPool* return_verticies, UintPool* return_indicies)
{
	ifstream fin(file_name);

	if (fin.fail()) {
		cout << "파일이 열리지 않습니다!" << endl;
		return;
		//Error
	}

	char _line[MAX_STRING_CHAR_COUNT];


	unsigned int tmp_token_count = 0;
	char* tmp_head = nullptr;
	char** _tokens = nullptr;
	char** tmp_indicies = nullptr;
	unsigned int tmp_indicies_count = 0;

	bool is_contained_unsuported = false;
	unsigned int vertex_count = 0;
	unsigned int vertex_normal_count = 0;
	unsigned int vertex_texture_count = 0;
	unsigned int face_count = 0;


	glm::vec3 vert;
	while (!fin.eof())    //파일 끝까지 읽었는지 확인
	{
		fin.getline(_line, MAX_STRING_CHAR_COUNT);    //한줄씩 읽어오기
		if (_line[0] == '#') cout << _line << endl;
		else {
			_tokens = returnSplit(_line, ' ', &tmp_token_count);
			tmp_head = _tokens[0];

			if (strcmp(tmp_head, "v") == 0) vertex_count++;
			else if (strcmp(tmp_head, "vn") == 0) vertex_normal_count++;
			else if (strcmp(tmp_head, "vt") == 0) vertex_texture_count++;
			else if (strcmp(tmp_head, "f") == 0) {
				//unsigned int vIndicies[3], vtIndicies[3], vnIndicies[3];
			}
			else if (strcmp(tmp_head, "o") == 0) {
				//오브젝트 이름 등록
				char object_name[100];
				strcpy_s(object_name, 100, _tokens[1]);
				cout << "오브젝트 이름 : " << object_name << endl;
			}
			else if (strcmp(tmp_head, "g") == 0) {
				char group_name[MAX_STRING_CHAR_COUNT];
				strcpy_s(group_name, MAX_STRING_CHAR_COUNT, _tokens[1]);
				cout << "그룹명 : " << group_name << endl;
			}
			else if (strcmp(tmp_head, "s") == 0) {

				bool is_smooth_shaded = false;
				if (strcmp(_tokens[1], "on") == 0) {
					is_smooth_shaded = true;
				}
			}
			else if (strcmp(tmp_head, "mtllib") == 0) {
				char material_file_name[MAX_STRING_CHAR_COUNT];
				strcpy_s(material_file_name, MAX_STRING_CHAR_COUNT, _tokens[1]);
				cout << "머티리얼 파일 이름 : " << material_file_name << endl;
			}
			else if (strcmp(tmp_head, "usemtl") == 0) {
				char material_name[MAX_STRING_CHAR_COUNT];
				strcpy_s(material_name, MAX_STRING_CHAR_COUNT, _tokens[1]);
				cout << "머티리얼명 : " << material_name << endl;
			}
			else {
				is_contained_unsuported = true;
				cout << "비지원형식 : " << tmp_head << endl;
			}

			//토큰 동적 배열 할당해제
			for (int i = 0; i < tmp_token_count; i++) delete[] _tokens[i];
			delete[] _tokens;
		}

	}

	fin.close();
	fin.open(file_name);

	glm::vec3* verticies = new glm::vec3[vertex_count];
	glm::vec3* vertex_normals = new glm::vec3[vertex_normal_count];
	glm::vec2* vertex_textures = new glm::vec2[vertex_texture_count];


	unsigned int vertex_index = 0;
	unsigned int vertex_normal_index = 0;
	unsigned int vertex_texture_index = 0;
	unsigned int face_index = 0;
	while (!fin.eof())    //파일 끝까지 읽었는지 확인
	{
		fin.getline(_line, MAX_STRING_CHAR_COUNT);    //한줄씩 읽어오기
		if (_line[0] != '#') {
			_tokens = returnSplit(_line, ' ', &tmp_token_count);
			tmp_head = _tokens[0];

			if (strcmp(tmp_head, "v") == 0) { //어떤 경우에도 룩업테이블로 유도해야댐
				float v[3];
				for (int i = 1; i <= 3; i++) v[i - 1] = atof(_tokens[i]);
				verticies[vertex_index++] = glm::vec3(v[0], v[1], v[2]);
			}
			else if (strcmp(tmp_head, "vn") == 0) {
				float vn[3];

				for (int i = 1; i <= 3; i++) vn[i - 1] = atof(_tokens[i]);
				vertex_normals[vertex_normal_index++] = glm::vec3(vn[0], vn[1], vn[2]);

			}
			else if (strcmp(tmp_head, "vt") == 0) {
				float vt[2];
				for (int i = 1; i <= 2; i++) vt[i - 1] = atof(_tokens[i]);
				vertex_textures[vertex_texture_index++] = glm::vec2(vt[0], vt[1]);

			}
			else if (strcmp(tmp_head, "f") == 0) {
				unsigned int vIndicies[3], vtIndicies[3], vnIndicies[3];

				for (int i = 1; i <= 3; i++) {

					tmp_indicies = returnSplit(_tokens[i], '/', &tmp_indicies_count);

					if (tmp_indicies_count == 3) {
						vIndicies[i - 1] = atoi(tmp_indicies[0]);
						vtIndicies[i - 1] = atoi(tmp_indicies[1]);
						vnIndicies[i - 1] = atoi(tmp_indicies[2]);
					}
					else {
						std::cout << "블렌더 Export세팅을 통해 삼각형 메시로 변환하세요(Triangulate)!" << endl;
					}
				}

				for (int i = 0; i < 3; i++)
				{
					unsigned int vertexindex = vIndicies[i] - 1;
					unsigned int normalindex = vnIndicies[i] - 1;
					unsigned int textureindex = vtIndicies[i] - 1;
					Vertex resultData = { verticies[vertexindex], glm::vec3(),vertex_textures[textureindex], vertex_normals[normalindex] };

					int target_index = 0;
					if (return_verticies->IsContain(resultData, &target_index))
					{
						return_indicies->Add(target_index);
					}
					else
					{
						return_indicies->Add(return_verticies->GetCount());
						return_verticies->Add(resultData);

					}
				}

			}

			//토큰 동적 배열 할당해제
			for (int i = 0; i < tmp_token_count; i++) delete[] _tokens[i];
			delete[] _tokens;
		}


	}


	delete[] verticies;
	delete[] vertex_normals;
	delete[] vertex_textures;
	fin.close();
}
