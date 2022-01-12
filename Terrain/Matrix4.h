#ifndef MATRIX4_H
#define MATRIX4_H

class Matrix4
{
public:
	Matrix4();
	Matrix4(
		float ie11, float ie12, float ie13, float ie14,
		float ie21, float ie22, float ie23, float ie24,
		float ie31, float ie32, float ie33, float ie34,
		float ie41, float ie42, float ie43, float ie44);
	Matrix4(const Matrix4 &m);
	~Matrix4();

	Matrix4 operator*(const Matrix4 &a);


	float e11; float e12; float e13; float e14;
	float e21; float e22; float e23; float e24;
	float e31; float e32; float e33; float e34;
	float e41; float e42; float e43; float e44;

private:
};

Matrix4 AdjustCoordinate();
Matrix4 PerspectiveProjection(float aspect_ratio, float field_of_view, float znear, float zfar);
Matrix4 OrthographicProjection(float bottom, float top, float left, float right, float znear, float zfar);
Matrix4 SimpleView(float xpos, float ypos, float zpos, float xrot, float yrot);
//미완성
Matrix4 LookAt(float xpos, float ypos, float zpos, float xdir, float ydir, float zdir, float xup, float yup, float zup);

Matrix4 Matrix4Identity();
Matrix4 Matrix4Multiplication(const Matrix4 *a, const Matrix4 *b);
Matrix4 Matrix4Translation(float x, float y, float z);
Matrix4 Matrix4Scale(float x, float y, float z);
//벡터의 크기는 1이어야 함
Matrix4 Matrix4AxisAngleRotation(float angle, float vx, float vy, float vz);


#endif // MATRIX4_H

