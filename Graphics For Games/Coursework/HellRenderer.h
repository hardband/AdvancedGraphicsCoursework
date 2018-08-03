#pragma once

#include "..\..\nclgl\OGLRenderer.h"
#include "..\..\nclgl\Camera.h"
#include "..\..\nclgl\HeightMap.h"
#include "..\..\nclgl\MD5Mesh.h"
#include "..\..\nclgl\MD5Node.h"
#include "..\..\nclgl\TextMesh.h"
#include "..\..\nclgl\PlaneEmitter.h"

#define SHADOWSIZE 2048 // New !

class HellRenderer : public OGLRenderer
{
public:
	HellRenderer(Window &parent);
	virtual ~HellRenderer();

	virtual void RenderScene();
	virtual void UpdateScene(float msec);

protected:

	//Draw methods
	void drawHeightMap();
	void drawSkyBox();
	void drawLava();
	void drawKnight();
	void drawSmoke();
	void drawReflect();
	void DrawText(const std::string &text, const Vector3 &position, const float size = 10.0f, const bool perspective = false);

	string setFramerate(float msec);

	Mesh* heightMap;
	Camera* camera;
	Light* light;

	//Shaders
	Shader* skyboxShader;
	Shader* terrainShader;
	Shader* lavaShader;
	Shader* knightShader;
	Shader* fontShader;
	Shader* particleShader;
	Shader* reflectShader;

	Mesh* skyboxQuad;
	Mesh* lavaQuad;
	Mesh* reflectQuad;

	GLuint cubeMap;

	//Hell Knight data
	MD5FileData * hellData;
	MD5Node * hellNode;

	Font* basicFont;

	string frameRate;

	//Smoke particle emmiter
	PlaneEmitter* smokeEmitter;



};

