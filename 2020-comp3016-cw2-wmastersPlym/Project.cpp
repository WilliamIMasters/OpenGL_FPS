//////////////////////////////////////////////////////////////////////////////
//
//  Triangles.cpp
//
//////////////////////////////////////////////////////////////////////////////

#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GLFW/glfw3.h"
#include "LoadShaders.h"
#include "glm/glm.hpp" //includes GLM
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/ext/matrix_transform.hpp" // GLM: translate, rotate
#include "glm/ext/matrix_clip_space.hpp" // GLM: perspective and ortho 
#include "glm/gtc/type_ptr.hpp" // GLM: access to the value_ptr

#include "assimp/include/assimp/Importer.hpp"
#include "assimp/include/assimp/scene.h"
#include "assimp/include/assimp/postprocess.h"



#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

#include <Model.hpp>

float deltaTime = 0.0f;
float lastFrame = 0.0f;



//----------------------------------------------------------------------------
//
// GameObject Class 
//
class GameObject {
public:
	std::string name;

	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

	bool isViewable = true;
	bool isStatic = true;
	bool isCollidableWithPlayer = true;
	bool isDestructable = false;

	std::vector<Model> models;

	HitBox hitBox;


	GameObject(std::string name, char* path) {
		this->name = name;

		Model model(path);
		models.push_back(model);
	}

	void addModel(char* path) {
		Model model(path);
		models.push_back(model);
	}


	void draw(GLuint program) {
		if (isViewable) {
			for (int i = 0; i < models.size(); i++) {
				models[i].Draw(program);
			}
		}
		

	}

	void destroy() {
		this->isViewable = false;
		this->isCollidableWithPlayer = false;
		this->isDestructable = false;
		this->velocity = glm::vec3(0.0f);
	}

};
std::vector<GameObject> scene;

//----------------------------------------------------------------------------
//
// Player class
//
class Player {
public:
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


	glm::vec3 direction;
	float pitch = 0.0f;
	float yaw = -90.0f;

	glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);

	float speed = 2.5f;
	float sensitivity = 2.5f;

	float fireBallSpeed = 8.0f;

	HitBox hitBox;

	Player() {

		hitBox.max = glm::vec3(0.4f, 0.4f, 0.4f);
		hitBox.min = glm::vec3(-0.4f, -0.4f, -0.4f);

		// Sets up fireball path // TODO load model and make a copy for optimisation
		fireBallPath = new char[fireBallPathstr.size() + 1];
		fireBallPathstr.copy(fireBallPath, fireBallPathstr.size() + 1);
		fireBallPath[fireBallPathstr.size()] = '\0';
		int a = 1;
	}
	
	void printPos() {
		std::string x = std::to_string(pos.x);
		std::cout << "X: " + std::to_string(pos.x) + ", Y: " + std::to_string(pos.y) + ", Z: " + std::to_string(pos.z) << std::endl;
		

	}

	void fireFireBall() {
		GameObject fireBall("FireBall",fireBallPath);
		fireBall.pos = this->pos;
		fireBall.isStatic = false;
		fireBall.velocity = this->cameraFront * fireBallSpeed;//glm::vec3(1.0f);
		fireBall.isCollidableWithPlayer = false;

		scene.push_back(fireBall);
	}

	void OnCollision(GameObject collider) {
		

		if (collider.isCollidableWithPlayer) {
			//float width = this->hitBox.max.x - this->hitBox.min.x;
			//float depth = this->hitBox.max.z - this->hitBox.min.z;

			std::cout << "Player colliding with: " + collider.name << std::endl;

			float xOverlap = ((this->hitBox.max.x - this->hitBox.min.x) + (collider.hitBox.max.x - collider.hitBox.min.x)) - glm::abs(this->pos.x - collider.pos.x);
			float zOverlap = ((this->hitBox.max.z - this->hitBox.min.z) + (collider.hitBox.max.z - collider.hitBox.min.z)) - glm::abs(this->pos.z - collider.pos.z);

			if (xOverlap < zOverlap) {
				float direction = this->pos.x < collider.pos.x ? 1 : -1;
				this->pos.x += xOverlap * direction;
			}
			else {
				float direction = this->pos.z < collider.pos.z ? 1 : -1;
				this->pos.z += zOverlap * direction;
			}

			//this->velocity *= -1; // Temp Sol -- Very buggy
			//this->pos += this->velocity * deltaTime;



		}
		
	}

private: 
	std::string fireBallPathstr = "./Assets/FireBall/FireBall.obj";
	char *fireBallPath;



};





// to use this example you will need to download the header files for GLM put them into a folder which you will reference in
// properties -> VC++ Directories -> libraries

enum VAO_IDs { Triangles, Indices, Colours, Tex, NumVAOs = 2  };
enum Buffer_IDs { ArrayBuffer, NumBuffers = 4 };
enum Attrib_IDs { vPosition = 0, cPosition = 1, tPosition = 2 };

GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint texture1;

const GLuint  NumVertices = 36;



Player player;

GLuint program;

int windowWidth = 800, windowHeight = 600;

float lastX = 400, lastY = 300;

//----------------------------------------------------------------------------
//
// HitBox's functions
//

bool CheckCollision(GameObject a, GameObject b) {

	glm::vec3 aMin = a.models.front().hitbox.min + a.pos;
	glm::vec3 aMax = a.models.front().hitbox.max + a.pos;

	glm::vec3 bMin = b.models.front().hitbox.min + b.pos;
	glm::vec3 bMax = b.models.front().hitbox.max + b.pos;

	return((aMin.x <= bMax.x && aMax.x >= bMin.x) && (aMin.y <= bMax.y && aMax.y >= bMin.y) && (aMin.z <= bMax.z && aMax.z >= bMin.z));
}

bool CheckCollisionWithPlayer(GameObject a) {

	glm::vec3 aMin = a.models.front().hitbox.min + a.pos;
	glm::vec3 aMax = a.models.front().hitbox.max + a.pos;

	glm::vec3 bMin = player.hitBox.min + player.pos;
	glm::vec3 bMax = player.hitBox.max + player.pos;

	return((aMin.x <= bMax.x && aMax.x >= bMin.x) && (aMin.y <= bMax.y && aMax.y >= bMin.y) && (aMin.z <= bMax.z && aMax.z >= bMin.z));
}

//----------------------------------------------------------------------------
//
// init
//
#define BUFFER_OFFSET(a) ((void*)(a))

void loadTexture(GLuint& texture, std::string texturepath)
{
	// load and create a texture 
// -------------------------

// texture 1
// ---------
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	GLint width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(texturepath.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

std::vector<GameObject> initScene(std::vector<GameObject> sceneInit) {


	//std::string path = fs::current_path().string();
	//char pathChar[path.size()] = path;
	// Create Enviroment Objects
	//
	//std::string floorLoc = path + "Assets/env/Floor/floor.obj";

	

	char floorPath[] = "./Assets/env/Floor/floor.obj";
	GameObject envFloor("envFloor", floorPath);
	envFloor.isCollidableWithPlayer = false;
	sceneInit.push_back(envFloor);

	char wallPlanePath[] = "./Assets/env/WallPlane/wallPlain.obj";
	GameObject wallPlaneA("wallPlaneA", wallPlanePath);
	wallPlaneA.pos.x = -5.0f;
	wallPlaneA.isCollidableWithPlayer = false;
	sceneInit.push_back(wallPlaneA);

	GameObject wallPlaneB("wallPlaneB", wallPlanePath);
	wallPlaneB.pos.x = 5.0f;
	wallPlaneB.isCollidableWithPlayer = false;
	sceneInit.push_back(wallPlaneB);

	char wallDoorPath[] = "./Assets/env/WallDoor/wallDoor.obj";
	GameObject wallDoorA("wallDoorA", wallDoorPath);
	wallDoorA.pos.z = -5.0f;
	wallDoorA.isCollidableWithPlayer = false;
	sceneInit.push_back(wallDoorA);

	GameObject wallDoorB("wallDoorB", wallDoorPath);
	wallDoorB.pos.z = 5.0f;
	wallDoorB.isCollidableWithPlayer = false;
	sceneInit.push_back(wallDoorB);

	/*// Create barrel Object
	char barrelPath[] = "./Assets/barrel/barrel.obj";
	GameObject barrel("barrel",barrelPath);
	barrel.isDestructable = true;
	sceneInit.push_back(barrel);

	char barrelBPath[] = "./Assets/barrel/barrel.obj";
	GameObject barrelB("barrelB", barrelBPath);
	barrelB.isDestructable = true;
	barrelB.pos = glm::vec3(2.0f, 0.0f, 0.0f);
	sceneInit.push_back(barrelB);*/

	char barrelPath[] = "./Assets/barrel/barrel.obj";
	for (int i = -3; i < 3; i++) {
		for (int ii = 0; ii < 2; ii++) {
			GameObject barrelB(("barrel" + i + ii), barrelPath);
			barrelB.isDestructable = true;
			barrelB.pos = glm::vec3((float)i, 0.0f, (float)ii);
			sceneInit.push_back(barrelB);
		}
	}



	return sceneInit;
}

void init(void)
{
	//glGenVertexArrays(NumVAOs, VAOs);
	//glBindVertexArray(VAOs[Triangles]);

	ShaderInfo  shaders[] =
	{
		{ GL_VERTEX_SHADER, "media/triangles.vert" },
		{ GL_FRAGMENT_SHADER, "media/triangles.frag" },
		{ GL_NONE, NULL }
	};

	program = LoadShaders(shaders);
	glUseProgram(program);

	scene = initScene(scene);

	/*GLfloat vertices[][3] = {
		{0.5f,  0.5f, -0.5f},  //0 top right
		{0.5f, -0.5f, -0.5f},  //1 bottom right
		{-0.5f, -0.5f, -0.5f}, //2 bottom left
		{-0.5f,  0.5f, -0.5f},  //3 top left

		{0.5f,  0.5f, 0.5f},  //4 top right
		{0.5f, -0.5f, 0.5f},  //5 bottom right
		{-0.5f, -0.5f, 0.5f}, //6 bottom left
		{-0.5f,  0.5f, 0.5f}  //7 top left 
	};
	GLuint indices[][3] = {  // note that we start from 0!
		{0, 3, 1},  // first Triangle front
		{3, 2, 1},   // second Triangle
		
		{4, 7, 0 },
		{7, 3, 0 },
		
		{1, 2, 5 },
		{2, 6, 5 },
		
		{5, 4, 0 },
		{0, 1, 5 },
		
		{2, 3, 7 },
		{7, 6, 2 },
		
		{4, 5, 7 },  // first Triangle back
		{7, 5, 6 }   // second Triangle
	};

	GLfloat  colours[][4] = {
		{ 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f },  
		{ 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, 
		{ 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f }, 
	};
	GLfloat  texture_coords[] = {
		 1.0f, 1.0f,
		 1.0f, 0.0f,
		 0.0f, 0.0f,
		 0.0f, 1.0f,

		 0.0f, 1.0f,
	     0.0f, 0.0f,
		 1.0f, 0.0f,
		 1.0f, 1.0f,

	};



	glGenBuffers(NumBuffers, Buffers);
	
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[Triangles]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffers[Indices]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	

	glVertexAttribPointer(vPosition, 3, GL_FLOAT,
		GL_FALSE, 0, BUFFER_OFFSET(0));
	
	//Colour Binding
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[Colours]);
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(colours), colours, 0);


	glVertexAttribPointer(cPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	//Texture Binding
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[Tex]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texture_coords), texture_coords, GL_STATIC_DRAW);
	glVertexAttribPointer(tPosition, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	*/


	/*loadTexture(texture1, "media/textures/awesomeface2.png");
	glUniform1i(glGetUniformLocation(program, "texture1"), 0);



	glEnableVertexAttribArray(vPosition);
	glEnableVertexAttribArray(cPosition); 
	glEnableVertexAttribArray(tPosition);*/
}




//----------------------------------------------------------------------------
//
// Register Input
//
// Keyboard and mouse
void key_callback(GLFWwindow* window)
{
	
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
		
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		//player.printPos();
		player.fireFireBall();
		printf("E");
	}

	/*if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		
		std::vector<GameObject> barrels;

		for (int i = 0; i < scene.size(); i++) {
			if (scene[i].name == "barrel" || scene[i].name == "barrelB") {
				std::cout << scene[i].name + " pos: X: " + std::to_string(scene[i].pos.x) + ", Y: " + std::to_string(scene[i].pos.y) + ", Z: " + std::to_string(scene[i].pos.z) << std::endl;
				barrels.push_back( scene[i]);
			}
		}

		
		std::string collidingB = CheckCollision(barrels[0], barrels[1]) ? "true" : "false";
		std::cout << "Barrel and BarrelB colliding: " + collidingB << std::endl;

		//std::string collidingP = CheckCollisionWithPlayer(barrels[0]) ? "true" : "false";
		//std::cout << "Barrel and player colliding: " + collidingP << std::endl;
	}*/

	/*if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {

		for (int i = 0; i < scene.size(); i++) {
			if (scene[i].name == "wallPlaneA") {
				scene[i].pos.x -= 0.05f;
				std::cout << std::to_string(scene[i].pos.x) << std::endl;
			}
		}
	}*/

	player.velocity = glm::vec3(0.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { // Forwards
		//player.pos += player.speed * player.cameraFront;
		//glm::vec3 forward = v
		player.velocity.x = player.speed * player.cameraFront.x;// 
		player.velocity.z = player.speed * player.cameraFront.z;// 
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { // Backwards
		player.velocity.x -= player.speed * player.cameraFront.x;
		player.velocity.z -= player.speed * player.cameraFront.z;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { // Left
		player.velocity -= glm::normalize(glm::cross(player.cameraFront, player.cameraUp)) * player.speed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { // Right
		player.velocity += glm::normalize(glm::cross(player.cameraFront, player.cameraUp)) * player.speed;
	}
		
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	float xDif = xpos - lastX;
	float yDif = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	xDif *= player.sensitivity *deltaTime;
	yDif *= player.sensitivity *deltaTime;

	player.yaw += xDif;
	player.pitch += yDif;

	if (player.pitch > 89.f) {
		player.pitch = 89.0f;
	}
	if (player.pitch < -89.f) {
		player.pitch = -89.0f;
	}

	glm::vec3 direction;
	direction.x = cos(glm::radians(player.yaw)) * cos(glm::radians(player.pitch));
	direction.y = sin(glm::radians(player.pitch));
	direction.z = sin(glm::radians(player.yaw)) * cos(glm::radians(player.pitch));
	player.cameraFront = glm::normalize(direction);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//----------------------------------------------------------------------------
//
// display
//
void display(void)
{
	static const float sky[] = { 0.53f, 0.81f, 0.92f, 0.0f };
	//static const GLfloat skyC[] = { 0.53f, 0.81f, 0.92f, 0.0f };

	//glClearColor(skyC);

	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearBufferfv(GL_COLOR, 0, sky);


	

	//glm::mat4 projection = glm::perspective(45.0f, 4.0f / 3, 0.1f, 20.0f);
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
	int projectionLoc = glGetUniformLocation(program, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//adding the view and projection to the shader
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::lookAt(player.pos, player.pos + player.cameraFront, player.cameraUp);

	int viewLoc = glGetUniformLocation(program, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


	// Camera work
	for (int i = 0; i < scene.size(); i++) {
		glm::mat4 model = glm::mat4(1.0f);

		

		model = glm::translate(model, scene[i].pos);

		

		int modelLoc = glGetUniformLocation(program, "model");
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		scene[i].draw(program);

	}



	
	
	
	//int mvpLoc = glGetUniformLocation(program, "mvp");
	//glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

	



	// bind textures on corresponding texture units
	/*glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glBindVertexArray(VAOs[Triangles]);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glDrawElements(GL_TRIANGLES, NumVertices, GL_UNSIGNED_INT, 0);*/
	


	
}

//----------------------------------------------------------------------------
//
// Movement And collisions
//
void calcMovementAndCol() {



	// Check for collisions with player
	for (int i = 0; i < scene.size(); i++) {
		if (CheckCollisionWithPlayer(scene[i])) { // If there is a collision
			player.OnCollision(scene[i]);
		}
	}

	player.pos += player.velocity * deltaTime;

	// Keeps player within the scene
	float limit = 4.5f;
	if (player.pos.x > limit) {
		player.pos.x = limit;
	}
	else if(player.pos.x < -limit){
		player.pos.x = -limit;
	}
	if (player.pos.z > limit) {
		player.pos.z = limit;
	}
	else if (player.pos.z < -limit) {
		player.pos.z = -limit;
	}

	// Calc movement for objects in scene
	for (int i = 0; i < scene.size(); i++) {
		if (!scene[i].isStatic) {
			scene[i].pos += scene[i].velocity * deltaTime;
			
			// if Objects a Fireball checks for other collisions
			if (scene[i].name == "FireBall") {
				for (int ii = 0; ii < scene.size(); ii++) {
					if (CheckCollision(scene[i], scene[ii]) && scene[ii].isDestructable) {
						scene[ii].destroy();
						scene[i].destroy();
						//scene
					}
				}
			}
			


			//scene[i].moveHitBox();
		}
	}
}




//----------------------------------------------------------------------------
//
// main
//
int
main(int argc, char** argv)
{
	

	glfwInit();

	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Textured Cube", NULL, NULL);
	glfwMakeContextCurrent(window);
	//glfwSetKeyCallback(window, key_callback);

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Hides mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Sets mouse callback function
	glfwSetCursorPosCallback(window, mouse_callback);
	glEnable(GL_DEPTH_TEST);

	glewInit();

	player.printPos();

	init();
	
	std::cout << "All good" << std::endl;
	

	/*// Create Enviroment Object
	char envPath[] = "E:/Documents/GitHub/2020-comp3016-cw2-wmastersPlym/Assets/env/TestEnv.obj";
	bool exists = fs::exists(envPath);
	Model env(envPath);
	

	// Create barrel Object
	char barrelPath[] = "E:/Documents/GitHub/2020-comp3016-cw2-wmastersPlym/Assets/barrel/barrel.obj";
	//bool exists = fs::exists(path);
	Model barrel(barrelPath);*/
	

	while (!glfwWindowShouldClose(window))
	{
		// Calculate deltaTime
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		

		// Input
		key_callback(window);


		// Calc movement 
		calcMovementAndCol();

		// Display
		display();

		
		// creates loop
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();
}
