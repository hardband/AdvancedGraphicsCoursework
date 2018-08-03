#pragma once

#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/SceneNode.h"
#include "../../nclgl/Frustum.h"
#include "../../nclgl/CubeRobot.h"
#include "..\..\nclgl\HeightMap.h"
#include "../../nclgl/TextMesh.h"
#include <algorithm>


class CubeRenderer : public OGLRenderer
{
public:
	CubeRenderer(Window & parent);
	virtual ~CubeRenderer(void);

	virtual void UpdateScene(float msec);
	virtual void RenderScene();

protected:
	//Scene node methods
	void BuildNodeLists(SceneNode * from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode * n);

	//Miscelanious draw methods
	void drawSkyBox();
	void drawNodeScene();
	void drawTerrain();
	void DrawText(const std::string &text, const Vector3 &position, const float size = 10.0f, const bool perspective = false);

	void DrawPostProcess();
	void presentScene();

	string setFramerate(float msec);

	//Shaders
	Shader* skyboxShader;
	Shader* sceneShader;
	Shader* terrainShader;
	Shader* fontShader;
	Shader* processShader;

	SceneNode * root;
	Camera * camera;

	//Meshes
	Mesh* processQuad;
	Mesh * quad;
	Mesh* skyboxQuad;
	Mesh* cube;
	Mesh* heightMap;
	
	Frustum frameFrustum;

	GLuint cubeMap;

	//Node lists
	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	Font* basicFont;

	string frameRate;

	int postPasses;
	float inputBuffer = 0;

	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex;
};

