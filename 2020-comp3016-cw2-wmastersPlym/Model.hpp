#include "GL/glew.h"
#include "GL/freeglut.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp" //includes GLM
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/ext/matrix_transform.hpp" // GLM: translate, rotate
#include "glm/ext/matrix_clip_space.hpp" // GLM: perspective and ortho 
#include "glm/gtc/type_ptr.hpp" // GLM: access to the value_ptr



#include "assimp/include/assimp/Importer.hpp"
#include "assimp/include/assimp/cimport.h"
#include "assimp/include/assimp/scene.h"
#include "assimp/include/assimp/postprocess.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#include <iostream>
#include <vector>
#include <string>



enum Attrib_ID { vPos = 0, cPos = 1, tPos = 2 };


//----------------------------------------------------------------------------
//
// Structures used for storing Bounding box values
//
struct HitBox {
	glm::vec3 min;
	glm::vec3 max;
};

//----------------------------------------------------------------------------
//
// Structures used for storing assimp imported models
//

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct Texture {
	unsigned int id;
	std::string type;
	std::string path;
};

class Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) {
		this->vertices = vertices;
		this->indices = indices;
		this->textures = textures;

		setupMesh();
	}

	void Draw(GLuint program) {
		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		//std::cout << "textures.size()" + std::to_string(textures.size()) << std::endl;
		for (unsigned int i = 0; i < textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			std::string number;
			std::string name = textures[i].type;
			//std::cout << "Texture Name: " + name << std::endl;
			if (name == "texture_diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "texture_specular")
				number = std::to_string(specularNr++);

			//shader.setFloat(("material." + name + number).c_str(), i);
			glUniform1i(glGetUniformLocation(program, ("material." + name + number).c_str()), i);
			glBindTexture(GL_TEXTURE_2D, textures[i].id);
		
			glEnableVertexAttribArray(vPos);
			glEnableVertexAttribArray(cPos);
			glEnableVertexAttribArray(tPos);
		}
		glActiveTexture(GL_TEXTURE0);

		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

	};

	

private:
	unsigned int VAO, VBO, EBO;

	void setupMesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));

		glBindVertexArray(0);
	}

};



class Model {
public:

	std::vector<Texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;

	HitBox hitbox;

	Model(char* path)
	{
		std::string pathstr;
		pathstr.append(path);

		std::cout << "Model: " + pathstr << std::endl;
		loadModel(path);

		CreateHitBox();
	}
	Model() = default;
	void Draw(GLuint program)
	{
		for (int i = 0; i < meshes.size(); i++) {
			meshes[i].Draw(program);
		}
	}

	void CreateHitBox() {
		std::vector<Vertex> vertices;
		for (int i = 0; i < meshes.size(); i++) {
			vertices.insert(vertices.end(), meshes[i].vertices.begin(), meshes[i].vertices.end());
		}

		hitbox.max = vertices[0].position; 
		hitbox.min = vertices[0].position;

		for (int i = 0; i < vertices.size(); i++) {
			// X
			if (vertices[i].position.x < hitbox.min.x) 
				hitbox.min.x = vertices[i].position.x;
			if (vertices[i].position.x > hitbox.max.x)
				hitbox.max.x = vertices[i].position.x;

			// Y
			if (vertices[i].position.y < hitbox.min.y)
				hitbox.min.y = vertices[i].position.y;
			if (vertices[i].position.y > hitbox.max.y)
				hitbox.max.y = vertices[i].position.y;

			// Z
			if (vertices[i].position.z < hitbox.min.z)
				hitbox.min.z = vertices[i].position.z;
			if (vertices[i].position.z > hitbox.max.z)
				hitbox.max.z = vertices[i].position.z;
			
		}


	}

private:


	void loadModel(char* path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);


		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "ASSIMP ERROR -> Model.h -> Model -> loadModel()" << std::endl;
			std::string err = importer.GetErrorString();
			std::cout << "ASSIMP ERROR: " + err << std::endl;
			return;
		}
		std::string pathstr;
		pathstr.append(path);

		directory = pathstr.substr(0, pathstr.find_last_of('/'));


		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		// Process all node's meshes
		for (int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		// Recursively processes all children nodes
		for (int i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<Vertex> verticies;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			glm::vec3 vector;


			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.normal = vector;
			}

			if (mesh->mTextureCoords[0]) { // Checks if mesh contains texture coords
				//std::cout << "textureCoordsTrue" << std::endl;
				
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.texCoords = vec;
			}
			else {
				vertex.texCoords = glm::vec2(0.0f, 0.0f);
			}


			verticies.push_back(vertex);

		}

		for (int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (int ii = 0; ii < face.mNumIndices; ii++) {
				indices.push_back(face.mIndices[ii]);
			}
		}

		std::cout << "mesh->mMaterialIndex: " + std::to_string(mesh->mMaterialIndex) << std::endl;

		if (mesh->mMaterialIndex >= 0) {
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}


		return Mesh(verticies, indices, textures);
	}

	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
	{
		std::vector<Texture> textures;
		for (int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString str;
			mat->GetTexture(type, i, &str);

			bool skip = false;

			for (int ii = 0; ii < textures_loaded.size(); ii++) {
				if (std::strcmp(textures_loaded[ii].path.data(), str.C_Str()) == 0) {
					textures.push_back(textures_loaded[ii]);
					skip = true;
					break;
				}
			}
			if (!skip) {
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
			}

		}
		return textures;
	}

	unsigned int TextureFromFile(const char* path, const std::string& directory) {
		std::string filename = std::string(path);
		filename = directory + '/' + filename;

		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;

		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			GLenum format;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;
			else if (nrComponents == 4)
				format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}




};
