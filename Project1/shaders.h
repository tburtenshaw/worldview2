#pragma once

#include <glew.h>
#include <GLFW/glfw3.h>
#include <sstream>



class Shader {
public:
	Shader();
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint geometryShader;
	
	GLuint program;

	void UseMe();
	GLuint LoadShaderFromFile(const char* filename, GLenum type);
	GLuint CreateProgram();
	void SetUniformFromFloats(const char* uniformname, float f1);
	void SetUniformFromFloats(const char* uniformname, float f1,float f2);
	void SetUniformFromFloats(const char* uniformname, float f1, float f2, float f3);
	void SetUniformFromFloats(const char* uniformname, float f1, float f2, float f3, float f4);
private:
	GLboolean CheckForErrors(GLuint shader, GLuint type);
};