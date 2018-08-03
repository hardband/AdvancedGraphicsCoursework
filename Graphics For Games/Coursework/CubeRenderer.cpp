#include "CubeRenderer.h"

#define SET_SEED 12345
#define NUM_CUBES 150

CubeRenderer::CubeRenderer(Window & parent) : OGLRenderer(parent)
{
	CubeRobot::CreateCube();
	projMatrix = Matrix4::Perspective(1.0f, 100000.0f, (float)width / (float)height, 45.0f);

	camera = new Camera();
	camera->SetPosition(Vector3(100, 100, 2000.0f));

	postPasses = 3;

	//Initialise shaders
	sceneShader = new Shader(SHADERDIR"SceneVertex.glsl", SHADERDIR"SceneFragment.glsl");
	skyboxShader = new Shader(SHADERDIR"skyboxVertex.glsl", SHADERDIR"skyboxFragment.glsl");
	terrainShader = new Shader(SHADERDIR"texturedVertex.glsl", SHADERDIR"texturedFragment.glsl");
	fontShader = new Shader(SHADERDIR"fontVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	processShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"processFrag.glsl");

	heightMap = new HeightMap(TEXTUREDIR"cubeTown.raw");

	//Texture from: http://www.tutorialsforblender3d.com/sites/default/files/Textures/Metal-Tarnished/Textured/PersianMetal_256.png
	heightMap->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"metal.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	quad = Mesh::GenerateQuad();
	quad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"purple_swirl.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0));

	processQuad = Mesh::GenerateQuad();

	skyboxQuad = Mesh::GenerateQuad();

	//Skybox texture from: http://www.custommapmakers.org/skyboxes.php
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"vr_rt.tga",
		TEXTUREDIR"vr_lf.tga",
		TEXTUREDIR"vr_up.tga",
		TEXTUREDIR"vr_dn.tga",
		TEXTUREDIR"vr_bk.tga",
		TEXTUREDIR"vr_ft.tga",
		SOIL_LOAD_RGB,
		SOIL_CREATE_NEW_ID, 0
	);

	OBJMesh* m = new OBJMesh();
	m->LoadOBJMesh(MESHDIR"cube.obj");
	cube = m;

	//Link shaders
	if (!sceneShader->LinkProgram() || !skyboxShader->LinkProgram() || !terrainShader->LinkProgram() || !fontShader->LinkProgram() || !processShader->LinkProgram()||!quad->GetTexture() || !heightMap->GetTexture()|| !cubeMap) {
		return;
	}

	root = new SceneNode();

	//Create portals from quads
	SceneNode * portal = new SceneNode();
	portal->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
	portal->SetTransform(Matrix4::Translation(Vector3(0, 500, 0)) * Matrix4::Scale(Vector3(10,10,10)));
	portal->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
	portal->SetBoundingRadius(10000.0f);
	portal->SetMesh(quad);

	root->AddChild(portal);

	portal = new SceneNode();

	portal->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
	portal->SetTransform(Matrix4::Translation(Vector3(0, 500, 0)) * Matrix4::Scale(Vector3(10, 10, 10))* Matrix4::Rotation(90, Vector3(0, 1, 0)));
	portal->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
	portal->SetBoundingRadius(10000.0f);
	portal->SetMesh(quad);

	root->AddChild(portal);

	srand(SET_SEED);

	//Creates an amount of cubes of random sizes in random places
	for (int i = 0; i < NUM_CUBES; ++i) {
		SceneNode * s = new SceneNode();
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		int posRange = 50000;
		int scale = rand() % 10 + 1;

		s->SetTransform(Matrix4::Translation(Vector3((rand() % posRange) - (posRange/2) , (rand() % posRange) - (posRange / 2), (rand() % posRange) - (posRange / 2))) 
			* Matrix4::Scale(Vector3(scale,scale,scale)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetBoundingRadius(5000.0f);
		s->SetMesh(cube);
		root->AddChild(s);
	}

	root->AddChild(new CubeRobot());

	CubeRobot* giant = new CubeRobot();

	giant->SetTransform(Matrix4::Translation(Vector3(0.0f, 0.0f, -8000.0f)) * Matrix4::Scale(Vector3(50.0f, 50.0f, 50.0f)));
	giant->SetModelScale(Vector3(500.0f, 500.0f, 500.0f));

	root->AddChild(giant);

	SetTextureRepeating(heightMap->GetTexture(), true);

	basicFont = new Font(SOIL_load_OGL_texture(TEXTUREDIR"tahoma.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT), 16, 16);

	//POST PROCESS START
	// Generate scene depth texture
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	// Generate colour texture
	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO); 
	glGenFramebuffers(1, &processFBO); 

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
	// We can check FBO attachment success using this command !
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//POST PROCESS END

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

CubeRenderer::~CubeRenderer()
{
	delete root;
	delete quad;
	delete camera;
	delete cube;
	delete skyboxQuad;

	//Delete shaders
	delete skyboxShader;
	delete sceneShader;
	delete terrainShader;
	delete fontShader;
	delete processShader;

	delete heightMap;
	CubeRobot::DeleteCube();

	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);

	currentShader = 0;
}

void CubeRenderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();

	//Updates frustrum
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	frameRate = setFramerate(msec);

	root->Update(msec);

	//Check inputs to change post processing passes
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_UP) && inputBuffer > 100) {
		
		postPasses++;

		inputBuffer = 0;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_DOWN) && inputBuffer > 100) {

		if (postPasses > 0) {
			postPasses--;
		}

		inputBuffer = 0;
	}
	
	inputBuffer = inputBuffer + msec;
}

void CubeRenderer::BuildNodeLists(SceneNode * from) {

	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition(); from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}

	for (vector < SceneNode * >::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i) {
		BuildNodeLists((*i));
	}
}

void CubeRenderer::SortNodeLists() {
	std::sort(transparentNodeList.begin(), transparentNodeList.end(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void CubeRenderer::DrawNodes() {


	for (vector < SceneNode * >::const_iterator i = nodeList.begin(); i != nodeList.end(); ++i) {
		DrawNode((*i));
	}

	for (vector < SceneNode * >::const_reverse_iterator i = transparentNodeList.rbegin(); i != transparentNodeList.rend(); ++i) {
		DrawNode((*i));
	}


}

void CubeRenderer::DrawNode(SceneNode * n) {
	if (n->GetMesh()) {
		glUniformMatrix4fv(glGetUniformLocation(
			currentShader->GetProgram(), "modelMatrix"), 1, false,
			(float*)&(n->GetWorldTransform()* Matrix4::Scale(n->GetModelScale())));

		glUniform4fv(glGetUniformLocation(currentShader->GetProgram(),
			"nodeColour"), 1, (float *)& n->GetColour());

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(),
			"useTexture"), (int)n->GetMesh()->GetTexture());

		n->Draw(*this);
	}
}

void CubeRenderer::drawSkyBox()
{
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);

	UpdateShaderMatrices();
	skyboxQuad->Draw();

	glUseProgram(0);
	glDepthMask(GL_TRUE);
}

//Draws nodes 
void CubeRenderer::drawNodeScene()
{
	SetCurrentShader(sceneShader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(),"diffuseTex"), 0);
	DrawNodes();

	glUseProgram(0);
}

void CubeRenderer::drawTerrain()
{
	SetCurrentShader(terrainShader);

	modelMatrix = Matrix4::Translation(Vector3(-5000.0f, -3000.0f, -5000.0f)) * Matrix4::Scale(Vector3(2.0f, 8.0f, 2.0f));
	projMatrix = Matrix4::Perspective(1.0f, 100000.0f, (float)width / (float)height, 45.0f);

	UpdateShaderMatrices();

	heightMap->Draw();

	glUseProgram(0);
}

//Draws text to screen
void CubeRenderer::DrawText(const std::string & text, const Vector3 & position, const float size, const bool perspective)
{
	SetCurrentShader(fontShader);

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//Create a new temporary TextMesh, using our line of text and our font
	TextMesh* mesh = new TextMesh(text, *basicFont);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	if (perspective) {
		modelMatrix = Matrix4::Translation(position) * Matrix4::Scale(Vector3(size, size, 1));
		viewMatrix = camera->BuildViewMatrix();
		projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	}
	else {

		modelMatrix = Matrix4::Translation(Vector3(position.x, height - position.y, position.z)) * Matrix4::Scale(Vector3(size, size, 1));
		viewMatrix.ToIdentity();
		projMatrix = Matrix4::Orthographic(-1.0f, 1.0f, (float)width, 0.0f, (float)height, 0.0f);
	}
	//Update the matrices, and draw the mesh
	UpdateShaderMatrices();

	mesh->Draw();

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	projMatrix = Matrix4::Perspective(1.0f, 100000.0f, (float)width / (float)height, 45.0f);


	delete mesh; //Once it's drawn, we don't need it anymore!

	glUseProgram(0);
}

void CubeRenderer::DrawPostProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	SetCurrentShader(processShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	modelMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glUniform2f(glGetUniformLocation(currentShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	//Does a number of passes equal to current number
	for (int i = 0; i < postPasses; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 0);

		processQuad->SetTexture(bufferColourTex[0]);
		processQuad->Draw();
		// Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

		processQuad->SetTexture(bufferColourTex[1]);
		processQuad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	glEnable(GL_DEPTH_TEST);
}

//Presents the blurred scene
void CubeRenderer::presentScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	SetCurrentShader(sceneShader);
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	viewMatrix.ToIdentity();
	UpdateShaderMatrices();
	processQuad->SetTexture(bufferColourTex[0]);
	processQuad->Draw();
	glUseProgram(0);
	projMatrix = Matrix4::Perspective(1.0f, 100000.0f, (float)width / (float)height, 45.0f);
}

//Returns current framerate
string CubeRenderer::setFramerate(float msec)
{
	float frameRate = 1000 / msec;

	int intFrame = (int)frameRate;

	if ((int)(frameRate + 0.5) > intFrame) {
		intFrame++;
	}

	return to_string(intFrame);
}

void CubeRenderer::RenderScene() {
	//Build and sort node lists
	BuildNodeLists(root);
	SortNodeLists();

	//Set up post processing
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//Draw skybox
	drawSkyBox();
	//Draw the nodes in the node list
	drawNodeScene();
	//Draw terrain last to get cool effect with transparent quads
	drawTerrain();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Draw post processed effects
	DrawPostProcess();
	presentScene();

	//Draw framerate
	DrawText(frameRate, Vector3(0, 0, 0), 16.0f);

	SwapBuffers();
	ClearNodeLists();

}

void CubeRenderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}
