#ifndef CAMERA_H
#define CAMERA_H

#include "includes.h"
#include "framework.h"

class Camera
{
public:
	Vector3 eye; //where is the camera
	Vector3 center; //where is the camera pointing at
	Vector3 up; //reference for the up (not actual top vector)

	float fov;
	float aspect;
	float near_plane;
	float far_plane;

	Matrix44 view_matrix;	
	Matrix44 projection_matrix;
	Matrix44 viewprojection_matrix;

	Camera();

	void lookAt( Vector3 eye, Vector3 center, Vector3 up );
	void perspective( float fov, float aspect, float near_plane, float far_plane );

	Vector3 projectVector( Vector3 pos );

	void updateViewMatrix();
	void updateProjectionMatrix();

	Matrix44 getViewProjectionMatrix();
	Vector3 product(Vector3 a, Vector3 b);
};


#endif