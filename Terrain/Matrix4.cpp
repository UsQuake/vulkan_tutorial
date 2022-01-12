#include "Matrix4.h"

#include <math.h>

#define RAD_TO_DEG 0.0174533f

Matrix4::Matrix4()
{
}

Matrix4::Matrix4(
	float ie11, float ie12, float ie13, float ie14,
	float ie21, float ie22, float ie23, float ie24,
	float ie31, float ie32, float ie33, float ie34,
	float ie41, float ie42, float ie43, float ie44)
{
	e11 = ie11; e12 = ie12; e13 = ie13; e14 = ie14;
	e21 = ie21; e22 = ie22; e23 = ie23; e24 = ie24;
	e31 = ie31; e32 = ie32; e33 = ie33; e34 = ie34;
	e41 = ie41; e42 = ie42; e43 = ie43; e44 = ie44;
}

Matrix4::Matrix4(const Matrix4 &m)
{
	e11 = m.e11; e12 = m.e12; e13 = m.e13; e14 = m.e14;
	e21 = m.e21; e22 = m.e22; e23 = m.e23; e24 = m.e24;
	e31 = m.e31; e32 = m.e32; e33 = m.e33; e34 = m.e34;
	e41 = m.e41; e42 = m.e42; e43 = m.e43; e44 = m.e44;
}

Matrix4::~Matrix4()
{
}

Matrix4 Matrix4Identity()
{
	Matrix4 rv;
	rv.e11 = 1.0f; rv.e12 = 0.0f; rv.e13 = 0.0f; rv.e14 = 0.0f;
	rv.e21 = 0.0f; rv.e22 = 1.0f; rv.e23 = 0.0f; rv.e24 = 0.0f;
	rv.e31 = 0.0f; rv.e32 = 0.0f; rv.e33 = 1.0f; rv.e34 = 0.0f;
	rv.e41 = 0.0f; rv.e42 = 0.0f; rv.e43 = 0.0f; rv.e44 = 1.0f;

	return rv;
}


Matrix4 Matrix4::operator*(const Matrix4 &a)
{
	return Matrix4Multiplication(this, &a);
}

Matrix4 Matrix4Multiplication(const Matrix4 *a, const Matrix4 *b)
{
	Matrix4 rv;

	rv.e11 = a->e11*b->e11 + a->e12*b->e21 + a->e13*b->e31 + a->e14*b->e41;
	rv.e12 = a->e11*b->e12 + a->e12*b->e22 + a->e13*b->e32 + a->e14*b->e42;
	rv.e13 = a->e11*b->e13 + a->e12*b->e23 + a->e13*b->e33 + a->e14*b->e43;
	rv.e14 = a->e11*b->e14 + a->e12*b->e24 + a->e13*b->e34 + a->e14*b->e44;

	rv.e21 = a->e21*b->e11 + a->e22*b->e21 + a->e23*b->e31 + a->e24*b->e41;
	rv.e22 = a->e21*b->e12 + a->e22*b->e22 + a->e23*b->e32 + a->e24*b->e42;
	rv.e23 = a->e21*b->e13 + a->e22*b->e23 + a->e23*b->e33 + a->e24*b->e43;
	rv.e24 = a->e21*b->e14 + a->e22*b->e24 + a->e23*b->e34 + a->e24*b->e44;

	rv.e31 = a->e31*b->e11 + a->e32*b->e21 + a->e33*b->e31 + a->e34*b->e41;
	rv.e32 = a->e31*b->e12 + a->e32*b->e22 + a->e33*b->e32 + a->e34*b->e42;
	rv.e33 = a->e31*b->e13 + a->e32*b->e23 + a->e33*b->e33 + a->e34*b->e43;
	rv.e34 = a->e31*b->e14 + a->e32*b->e24 + a->e33*b->e34 + a->e34*b->e44;

	rv.e41 = a->e41*b->e11 + a->e42*b->e21 + a->e43*b->e31 + a->e44*b->e41;
	rv.e42 = a->e41*b->e12 + a->e42*b->e22 + a->e43*b->e32 + a->e44*b->e42;
	rv.e43 = a->e41*b->e13 + a->e42*b->e23 + a->e43*b->e33 + a->e44*b->e43;
	rv.e44 = a->e41*b->e14 + a->e42*b->e24 + a->e43*b->e34 + a->e44*b->e44;

	return rv;
}

Matrix4 Matrix4Translation(float x, float y, float z)
{
	Matrix4 rv;
	rv.e11 = 1.0f;
	rv.e12 = 0.0f;
	rv.e13 = 0.0f;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = 1.0f;
	rv.e23 = 0.0f;
	rv.e24 = 0.0f;

	rv.e31 = 0.0f;
	rv.e32 = 0.0f;
	rv.e33 = 1.0f;
	rv.e34 = 0.0f;

	rv.e41 = x;
	rv.e42 = y;
	rv.e43 = z;
	rv.e44 = 1.0f;

	return rv;
}

Matrix4 Matrix4Scale(float x, float y, float z)
{
	Matrix4 rv;
	rv.e11 = x;
	rv.e12 = 0.0f;
	rv.e13 = 0.0f;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = y;
	rv.e23 = 0.0f;
	rv.e24 = 0.0f;

	rv.e31 = 0.0f;
	rv.e32 = 0.0f;
	rv.e33 = z;
	rv.e34 = 0.0f;

	rv.e41 = 0.0f;
	rv.e42 = 0.0f;
	rv.e43 = 0.0f;
	rv.e44 = 1.0f;

	return rv;
}

Matrix4 Matrix4AxisAngleRotation(float angle, float vx, float vy, float vz)
{
	Matrix4 rv;
	float c = cosf(angle);
	float s = sinf(angle);

	rv.e11 = (1 - c)*vx*vx + c;
	rv.e12 = (1 - c)*vx*vy + s*vz;
	rv.e13 = (1 - c)*vx*vz - s*vy;
	rv.e14 = 0.0f;

	rv.e21 = (1 - c)*vx*vy - s*vz;
	rv.e22 = (1 - c)*vy*vy + c;
	rv.e23 = (1 - c)*vy*vz + s*vx;;
	rv.e24 = 0.0f;

	rv.e31 = (1 - c)*vx*vz + s*vy;
	rv.e32 = (1 - c)*vy*vz - s*vx;
	rv.e33 = (1 - c)*vz*vz + c;
	rv.e34 = 0.0f;

	rv.e41 = 0.0f;
	rv.e42 = 0.0f;
	rv.e43 = 0.0f;
	rv.e44 = 1.0f;

	return rv;
}

Matrix4 SimpleView(float xpos, float ypos, float zpos, float xrot, float yrot)
{
	Matrix4 rv;
	float cx = cosf(xrot);
	float cy = cosf(yrot);
	float sx = sinf(xrot);
	float sy = sinf(yrot);

	rv.e11 = cx;
	rv.e12 = sx*sy;
	rv.e13 = -cx*sy;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = cx;
	rv.e23 = sx;
	rv.e24 = 0.0f;

	rv.e31 = sy;
	rv.e32 = -sx*cy;
	rv.e33 = cx*cy;
	rv.e34 = 0.0f;

	rv.e41 = -xpos*cy - ypos*sx*sy + zpos*cx*sy;
	rv.e42 = -ypos*cx - zpos*sx;
	rv.e43 = -xpos*sy + ypos*sx*cy - zpos*cx*cy;
	rv.e44 = 1.0f;


	return rv;
}

Matrix4 LookAt(float xpos, float ypos, float zpos, float xdir, float ydir, float zdir, float xup, float yup, float zup)
{
	Matrix4 rv;
	rv.e11 = 1.0f;
	rv.e12 = 0.0f;
	rv.e13 = 0.0f;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = 1.0f;
	rv.e23 = 0.0f;
	rv.e24 = 0.0f;

	rv.e31 = 0.0f;
	rv.e32 = 0.0f;
	rv.e33 = 1.0f;
	rv.e34 = 0.0f;

	rv.e41 = -xpos;
	rv.e42 = -ypos;
	rv.e43 = -zpos;
	rv.e44 = 1.0f;

	return rv;
}

Matrix4 OrthographicProjection(float bottom, float top, float left, float right, float znear, float zfar)
{
	Matrix4 rv;
	rv.e11 = 2.0f/(right-left);
	rv.e12 = 0.0f;
	rv.e13 = 0.0f;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = 2.0f/(top-bottom);
	rv.e23 = 0.0f;
	rv.e24 = 0.0f;

	rv.e31 = 0.0f;
	rv.e32 = 0.0f;
	rv.e33 = -2.0f/(zfar-znear);
	rv.e34 = 0.0f;
	
	rv.e41 = -(right + left) / (right - left);
	rv.e42 = -(top + bottom) / (top - bottom);
	rv.e43 = -(zfar + znear) / (zfar - znear);
	rv.e44 = 1.0f;
	return rv;
}

Matrix4 PerspectiveProjection(float aspect_ratio, float field_of_view, float znear, float zfar)
{
	Matrix4 rv;
	float focal_distance = 1.0f/tanf(field_of_view / 2.0f);

	rv.e11 = focal_distance / aspect_ratio;
	rv.e12 = 0.0f;
	rv.e13 = 0.0f;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = focal_distance;
	rv.e23 = 0.0f;
	rv.e24 = 0.0f;

	rv.e31 = 0.0f;
	rv.e32 = 0.0f;
	rv.e33 = (znear + zfar) / (znear - zfar);
	rv.e34 = -1.0f;


	rv.e41 = 0.0f;
	rv.e42 = 0.0f;
	rv.e43 = (2.0f*znear*zfar)/(znear-zfar);
	rv.e44 = 0.0f;

	return rv;
}

Matrix4 AdjustCoordinate()
{
	Matrix4 rv;
	
	rv.e11 = 1.0f;
	rv.e12 = 0.0f;
	rv.e13 = 0.0f;
	rv.e14 = 0.0f;

	rv.e21 = 0.0f;
	rv.e22 = -1.0f;
	rv.e23 = 0.0f;
	rv.e24 = 0.0f;

	rv.e31 = 0.0f;
	rv.e32 = 0.0f;
	rv.e33 = 0.5f;
	rv.e34 = 0.0f;


	rv.e41 = 0.0f;
	rv.e42 = 0.0f;
	rv.e43 = 0.5f;
	rv.e44 = 1.0f;

	return rv;
}