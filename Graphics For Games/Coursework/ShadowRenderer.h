#pragma once

#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/MD5Mesh.h"
#include "../../nclgl/MD5Node.h"
#include"../../nclgl/OBJMesh.h"
#include "../../nclgl/TextMesh.h"
#include "../../nclgl/PlaneEmitter.h"

#define SHADOWSIZE 2048 // New !
#define LIGHT_SPEED 0.1f
#define MOVE_DISTANCE 2000.0f

class ShadowRenderer : public OGLRenderer
{
public:
	ShadowRenderer(Window & parent);
	virtual ~ShadowRenderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

protected:
	//Draw functions
	void DrawMesh();
	void DrawFloor();
	void DrawShadowScene(Light* source);
	void DrawCombinedScene(Light* source); 
	void DrawPillars();
	void DrawParticles();
	void DrawLightCube();
	void DrawText(const std::string &text, const Vector3 &position, const float size = 10.0f, const bool perspective = false);

	//Method to update the position on the light source to move it back and forth
	void updateLightPos(float msec);

	string setFramerate(float msec);

	//Shaders
	Shader * sceneShader;
	Shader * shadowShader;
	Shader* fontShader;
	Shader* particleShader;
	
	GLuint shadowTex;
	GLuint shadowFBO;

	MD5FileData * hellData;
	MD5Node * hellNode;
	Mesh * floor;
	Camera * camera;
	Light* light;
	Light* secondLight;
	Light* currentLight;

	Mesh* cube;

	ParticleEmitter* emitter;

	Font* basicFont;

	string frameRate;
	float inputBuffer = 0;

	bool forward = true;

};

