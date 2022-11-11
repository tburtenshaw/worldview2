#pragma once

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <sstream>

//forward declaration
class NSWE;

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
	GLuint LoadUniformLocation(unsigned int* l, const char* uniformname);

	void SetUniform(GLuint loc, float f1);
	void SetUniform(GLuint loc, float f1, float f2);
	void SetUniform(GLuint loc, int i1);
	void SetUniform(GLuint loc, const unsigned int i1);
	void SetUniform(GLuint loc, const unsigned long ul1);
	void SetUniform(GLuint loc, const NSWE* nswe);
	void SetUniform(GLuint loc, GLsizei count, const GLfloat* value);

	//deprecating these, as I wrote them before learning much about C++
	void SetUniformFromFloats(const char* uniformname, float f1);
	void SetUniformFromFloats(const char* uniformname, float f1,float f2);
	void SetUniformFromFloats(const char* uniformname, float f1, float f2, float f3);
	void SetUniformFromFloats(const char* uniformname, float f1, float f2, float f3, float f4);

	void SetUniformFromInts(const char* uniformname, int i1);
	void SetUniformFromNSWE(const char* uniformname, const NSWE* nswe);

private:
	GLboolean CheckForErrors(GLuint shader, GLuint type);
};