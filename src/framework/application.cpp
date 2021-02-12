#include "application.h"
#include "utils.h"
#include "image.h"
#include "mesh.h"
#include <algorithm>

Mesh* mesh = NULL;
Camera* camera = NULL;
Image* texture = NULL;
FloatImage zBuffer(800, 600);

Application::Application(const char* caption, int width, int height)
{
	this->window = createWindow(caption, width, height);

	// initialize attributes
	// Warning: DO NOT CREATE STUFF HERE, USE THE INIT 
	// things create here cannot access opengl
	int w,h;
	SDL_GetWindowSize(window,&w,&h);

	this->window_width = w;
	this->window_height = h;
	this->keystate = SDL_GetKeyboardState(NULL);

	framebuffer.resize(w, h);
}

//Here we have already GL working, so we can create meshes and textures
//Here we have already GL working, so we can create meshes and textures
void Application::init(void)
{
	std::cout << "initiating app..." << std::endl;
	
	//here we create a global camera and set a position and projection properties
	camera = new Camera();
	camera->lookAt(Vector3(0,10,20),Vector3(0,10,0),Vector3(0,1,0)); //define eye,center,up
	camera->perspective(60, window_width / (float)window_height, 0.1, 10000); //define fov,aspect,near,far

	//load a mesh
	mesh = new Mesh();
	if( !mesh->loadOBJ("lee.obj") )
		std::cout << "FILE Lee.obj NOT FOUND" << std::endl;

	//load the texture
	texture = new Image();
	texture->loadTGA("color.tga");
	zBuffer.fill(INFINITE);
}

//this function fills the triangle by computing the bounding box of the triangle in screen space and using the barycentric interpolation
//to check which pixels are inside the triangle. It is slow for big triangles, but faster for small triangles
void fillTriangle(Image& colorbuffer, const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, Image* texture, FloatImage* zbuffer)
{
	//compute triangle bounding box in screen space
	Vector3 min_, max_;
	computeMinMax(p0, p1, p2, min_, max_);
	//clamp to screen area
	min_ = clamp(min_, Vector3(0, 0, -1), Vector3(colorbuffer.width-1, colorbuffer.height-1, 1));
	max_ = clamp(max_, Vector3(0, 0, -1), Vector3(colorbuffer.width-1, colorbuffer.height-1, 1));

	//this avoids strange artifacts if the triangle is too big, just ignore this line
	if ((min_.x == 0.0 && max_.x == colorbuffer.width - 1) || (min_.y == 0.0 && max_.y == colorbuffer.height - 1))
		return;

	//we must compute the barycentrinc interpolation coefficients
	//we precompute some of them outside of loop to speed up (because they are constant)
	Vector3 v0 = p1 - p0;
	Vector3 v1 = p2 - p0;
	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float denom = d00 * d11 - d01 * d01;

	//loop all pixels inside bounding box
	for (int x = min_.x; x < max_.x; ++x)
	{
		#pragma omp parallel for //HACK: this is to execute loop iterations in parallel in multiple cores, should go faster (search openmp in google for more info)
		for (int y = min_.y; y < max_.y; ++y)
		{
			Vector3 P(x, y, 0);
			Vector3 v2 = P - p0; //P is the x,y of the pixel

			//computing all weights of pixel P(x,y)
			float d20 = v2.dot(v0);
			float d21 = v2.dot(v1);
			float v = (d11 * d20 - d01 * d21) / denom;
			float w = (d00 * d21 - d01 * d20) / denom;
			float u = 1.0 - v - w;
			//check if pixel is inside or outside the triangle
			if (u < 0 || u > 1 || v < 0 || v > 1 || w < 0 || w > 1)
				continue; //if it is outside, skip to next

			//here add your code to test occlusions based on the Z of the vertices and the pixel
			//...

			//here add your code to compute the color of the pixel
			//...
			float z = p0.z * u + p1.z * v + p2.z *w;

			if (z < zbuffer->getPixel(x, y)) {
				if (x < colorbuffer.width && x > 0 && y < colorbuffer.height && y > 0) {
					zbuffer->setPixel(x, y, z);
					Vector2 pixeltext = uv0 * u + uv1 * v + uv2 * w;

					Color c = texture->getPixel(pixeltext.x, pixeltext.y);
					//draw the pixels in the colorbuffer x,y position
					colorbuffer.setPixel(x, y, c);

				}
			}
		}
	}
}

//render one frame
void Application::render(Image& framebuffer)
{
	framebuffer.fill(Color(40, 45, 60 )); //clear
	
	//remember, you must modify the code in Camera::updateProjectionMatrix and Camera::updateViewMatrix to take into account changes in the camera

	//for every point of the mesh (to draw triangles take three points each time and connect the points between them (1,2,3,   4,5,6,   ... )
	for (unsigned int i = 0; i < mesh->vertices.size(); i+=3)
	{
		Vector3 v0 = mesh->vertices[i]; //extract vertex from mesh
		Vector3 v1 = mesh->vertices[i+1]; //extract vertex from mesh
		Vector3 v2 = mesh->vertices[i+2]; //extract vertex from mesh

		Vector2 uv0 = mesh->uvs[i]; //texture coordinate of the vertex (they are normalized, from 0,0 to 1,1)
		Vector2 uv1 = mesh->uvs[i+1]; //texture coordinate of the vertex (they are normalized, from 0,0 to 1,1)
		Vector2 uv2 = mesh->uvs[i+2]; //texture coordinate of the vertex (they are normalized, from 0,0 to 1,1)

		//project every point in the mesh to normalized coordinates using the viewprojection_matrix inside camera
		//you can use: projected_vertex = camera->projectVector(vertex);
		v0= camera->projectVector(v0);
		v1 = camera->projectVector(v1);
		v2 = camera->projectVector(v2);

		//convert from normalized (-1 to +1) to framebuffer coordinates (0..W,0..H)
		//CLIP-SPACE
		v0.x = (v0.x + 1)*framebuffer.width / 2;
		v0.y = (v0.y + 1)*framebuffer.height / 2;
		v1.x = (v1.x + 1)*framebuffer.width / 2;
		v1.y = (v1.y + 1)*framebuffer.height / 2;
		v2.x = (v2.x + 1)*framebuffer.width / 2;
		v2.y = (v2.y + 1)*framebuffer.height / 2;

		uv0.x = (uv0.x)*texture->width;
		uv0.y = (uv0.y)*texture->height;
		uv1.x = (uv1.x)*texture->width;
		uv1.y = (uv1.y)*texture->height;
		uv2.x = (uv2.x)*texture->width;
		uv2.y = (uv2.y)*texture->height;


		//paint point in framebuffer (using the fillTriangle function)
		//...
		//framebuffer.setPixel(v0.x,v0.y,Color::RED);
		//framebuffer.setPixel(v1.x, v1.y, Color::RED);
		//framebuffer.setPixel(v2.x, v2.y, Color::RED);
		fillTriangle(framebuffer, v0, v1, v2, uv0, uv1, uv2, texture, &zBuffer);

	}
	zBuffer.fill(INFINITE);
}

//called after render
void Application::update(double seconds_elapsed)
{
	//--------------EYE
	if (keystate[SDL_SCANCODE_UP]) {
		camera->eye.y += 5 * seconds_elapsed;
	}
	if (keystate[SDL_SCANCODE_DOWN]) {
		camera->eye.y -= 5 * seconds_elapsed;
	}
	//example to move eye
	if (keystate[SDL_SCANCODE_LEFT]) {
		camera->eye.x -= 5 * seconds_elapsed;
	}
	if (keystate[SDL_SCANCODE_RIGHT]) {
		camera->eye.x += 5 * seconds_elapsed;

	}

	//--------------CENTER
	if (keystate[SDL_SCANCODE_A]) {
		camera->center.x -= 5 * seconds_elapsed;
	}
	if (keystate[SDL_SCANCODE_D]) {
		camera->center.x += 5 * seconds_elapsed;
	}
	if (keystate[SDL_SCANCODE_W]) {
		camera->center.y += 5 * seconds_elapsed;
	}
	if (keystate[SDL_SCANCODE_S]) {
		camera->center.y -= 5 * seconds_elapsed;
	}

	//--------------------FOV
	if (keystate[SDL_SCANCODE_F]) {
		float aux = camera->fov + 5 * seconds_elapsed;
		if (20 < aux & aux < 100) {
			camera->fov += 5 * seconds_elapsed;
		}
	}
	if (keystate[SDL_SCANCODE_G]) {
		float aux = camera->fov - 5 * seconds_elapsed;
		if (20 < aux & aux < 100) {
			camera->fov -= 5 * seconds_elapsed;
		}

	}
	//------------------------------------


	//if we modify the camera fields, then update matrices
	camera->updateViewMatrix();
	camera->updateProjectionMatrix();
}

//keyboard press event 
void Application::onKeyDown( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: exit(0); break; //ESC key, kill the app
		default: break;
	}
}

//keyboard released event 
void Application::onKeyUp(SDL_KeyboardEvent event)
{
	//check onKeyDown to know how to use it
}

//mouse button event
void Application::onMouseButtonDown( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_LEFT) //left mouse pressed
	{

	}
}

void Application::onMouseButtonUp( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_LEFT) //left mouse unpressed
	{

	}
}

//executed when the windows changes its size
void Application::onWindowResize(int width, int height)
{
	framebuffer.resize(width, height);
	//something else should be resized?
	//...
}

//when the app starts
void Application::start()
{
	std::cout << "launching loop..." << std::endl;
	launchLoop(this);
}
