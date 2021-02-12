#include "camera.h"

Camera::Camera()
{
	this->fov = 45;
	this->aspect = 1;
	this->near_plane = 0.01;
	this->far_plane = 10000;

	eye = Vector3(0, 10, 20);
	center = Vector3(0, 10, 0);
	up = Vector3(0, 1, 0);

	//here I gave you two matrices so you can develop them independently
	//pay attention that they are column-major order [column][row]
	view_matrix.setIdentity();
	projection_matrix.setIdentity();
	viewprojection_matrix.setIdentity();

	updateViewMatrix();
	updateProjectionMatrix();

}

void Camera::updateViewMatrix()
{
	//IMPLEMENT THIS using eye, center and up, store in this->view_matrix
	//Careful with the order of matrix multiplications, and be sure to use normalized vectors

	Vector3 Front = operator-(center, eye);
	Front.normalize();
	Vector3 Side = product(Front, up);
	Side.normalize();
	Vector3 Top = product(Side, Front);

	view_matrix.setIdentity();

	this->view_matrix.M[0][0] = Side.x; this->view_matrix.M[0][1] = Top.x;		this->view_matrix.M[0][2] = -Front.x;	this->view_matrix.M[0][3] = 0.0;
	this->view_matrix.M[1][0] = Side.y; this->view_matrix.M[1][1] = Top.y;		this->view_matrix.M[1][2] = -Front.y;	this->view_matrix.M[1][3] = 0.0;
	this->view_matrix.M[2][0] = Side.z; this->view_matrix.M[2][1] = Top.z;		this->view_matrix.M[2][2] = -Front.z;	this->view_matrix.M[2][3] = 0.0;
	this->view_matrix.M[3][0] = 0.0;	this->view_matrix.M[3][1] = 0.0;		this->view_matrix.M[3][2] = 0.0;		this->view_matrix.M[3][3] = 1.0;
	view_matrix.translateLocal(-eye.x, -eye.y, -eye.z);

	////update the viewprojection_matrix
	viewprojection_matrix = view_matrix * projection_matrix;
}

void Camera::updateProjectionMatrix()
{
	//IMPLEMENT THIS using fov, aspect, near_plane and far_plane, store in this->projection_matrix
	//Careful with using degrees in trigonometric functions, must be radians, and use float types in divisions

	projection_matrix.setIdentity();
	float f = 1 / (tan(fov*DEG2RAD / 2));
	this->projection_matrix.M[0][0] = (f / aspect);
	this->projection_matrix.M[1][1] = f;
	this->projection_matrix.M[2][2] = ((far_plane + near_plane) / (near_plane - far_plane)); projection_matrix.M[2][3] = -1;
	this->projection_matrix.M[3][2] = (2 * ((far_plane * near_plane) / (near_plane - far_plane)));


	//update the viewprojection_matrix
	viewprojection_matrix = view_matrix * projection_matrix;
}

//this function projects a vertex using the viewprojection matrix
Vector3 Camera::projectVector( Vector3 pos )
{
	Vector4 pos4 = Vector4(pos.x, pos.y, pos.z, 1.0);
	Vector4 result = viewprojection_matrix * pos4;
	return result.getVector3() / result.w;
}

void Camera::lookAt( Vector3 eye, Vector3 center, Vector3 up )
{
	this->eye = eye;
	this->center = center;
	this->up = up;

	this->updateViewMatrix();
}

void Camera::perspective( float fov, float aspect, float near_plane, float far_plane )
{
	this->fov = fov;
	this->aspect = aspect;
	this->near_plane = near_plane;
	this->far_plane = far_plane;

	this->updateProjectionMatrix();
}

Matrix44 Camera::getViewProjectionMatrix()
{
	viewprojection_matrix = view_matrix * projection_matrix;
	return viewprojection_matrix ;
}

Vector3 Camera::product(Vector3 a, Vector3 b) {
	Vector3 aux;
	aux.x = a.y * b.z - b.y * a.z;
	aux.y = b.x * a.z - a.x * b.z;
	aux.z = a.x * b.y - a.y * b.x;
	return aux;
}

