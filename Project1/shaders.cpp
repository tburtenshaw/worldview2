#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "header.h"
#include "shaders.h"
#include "nswe.h"
#include <iostream>
#include <fstream>


Shader::Shader()
{
	vertexShader = fragmentShader = geometryShader = program = 0;
}

void Shader::UseMe() {
	glUseProgram(program);
	return;
}


GLuint Shader::LoadShaderFromFile(const char* filename, GLenum type) {

	GLuint shader;
	shader = glCreateShader(type);

	if (type == GL_FRAGMENT_SHADER) {
		fragmentShader = shader;
	}
	else if (type == GL_VERTEX_SHADER) {
		vertexShader = shader;
	}
	else if (type == GL_GEOMETRY_SHADER) {
		geometryShader = shader;
	}
	else return 0;
	
	std::ifstream file;
	file.open(filename);
	if (!file.is_open())
	{
		printf("Shader file %s not loadable\n", filename);

		char differentfolder[1000];
		sprintf_s(differentfolder, "..\\%s", filename);

		printf("Trying a directory up %s\n",differentfolder);
		file.open(differentfolder);

		if (!file.is_open()) {
			printf("nope\n");
			return 0;
		}
	}

	std::stringstream strStream;
	strStream << file.rdbuf(); //read the file
	std::string data = strStream.str(); //str holds the content of the file

	

	const char* c_str = data.c_str();

	glShaderSource(shader, 1, &c_str, NULL);
	glCompileShader(shader);
	printf("Shader file: %s\n", filename);
	if (int r=CheckForErrors(shader, GL_COMPILE_STATUS)) {
		printf("\nShader: %i,result:%i, ",shader, r);
		//printf("Content: % s \nFile : % s\n", data.c_str(), filename);
	}

	return shader;
}

GLuint Shader::CreateProgram()
{
	program= glCreateProgram();
	printf("Attaching: Ver..");
	glAttachShader(program, vertexShader);
	printf("Frag..");
	glAttachShader(program, fragmentShader);
	if (geometryShader) {
		printf("Geo..");
		glAttachShader(program, geometryShader);
	}
	printf("\nlinking program %i...\n",program);
	glLinkProgram(program);

	printf("Checking for linking errors:\n");
	CheckForErrors(program, GL_LINK_STATUS);

	return program;
}

GLuint Shader::LoadUniformLocation(unsigned int * l, const char* uniformname)
{
	*l= glGetUniformLocation(program, uniformname);
	return GLuint();
}

void Shader::SetUniform(GLuint loc, float f1)
{
	glUniform1f(loc, f1);
}

void Shader::SetUniform(GLuint loc, int i1)
{
	glUniform1i(loc, i1);
}


void Shader::SetUniform(GLuint loc, unsigned long ul1)
{
	glUniform1ui(loc, ul1);
}

void Shader::SetUniform(GLuint loc, float f1, float f2)
{
	glUniform2f(loc, f1, f2);
}

void Shader::SetUniform(GLuint loc, NSWE* nswe)
{
	glUniform4f(loc, nswe->north, nswe->south, nswe->west, nswe->east);
}

void Shader::SetUniform(GLuint loc, GLsizei count, const GLfloat* value)
{
	glUniform4fv(loc, count, value);
}


void Shader::SetUniformFromFloats(const char* uniformname, float f1)
{
	int uniloc;
	uniloc = glGetUniformLocation(program, uniformname);
	glUniform1f(uniloc, f1);
	return;
}
void Shader::SetUniformFromFloats(const char* uniformname, float f1, float f2)
{
	int uniloc;
	uniloc = glGetUniformLocation(program, uniformname);
	glUniform2f(uniloc, f1,f2);
	return;
}
void Shader::SetUniformFromFloats(const char* uniformname, float f1, float f2, float f3)
{
	int uniloc;
	uniloc = glGetUniformLocation(program, uniformname);
	glUniform3f(uniloc, f1, f2, f3);
	return;
}
void Shader::SetUniformFromFloats(const char* uniformname, float f1, float f2, float f3, float f4)
{
	int uniloc;
	uniloc = glGetUniformLocation(program, uniformname);
	glUniform4f(uniloc, f1, f2, f3, f4);
	return;
}

void Shader::SetUniformFromInts(const char* uniformname, int i1)
{
	int uniloc;
	uniloc = glGetUniformLocation(program, uniformname);
	glUniform1i(uniloc, i1);
}

void Shader::SetUniformFromNSWE(const char* uniformname, NSWE* nswe)
{
	int uniloc;
	uniloc = glGetUniformLocation(program, uniformname);
	glUniform4f(uniloc, nswe->north, nswe->south, nswe->west, nswe->east);
	return;
}

GLboolean Shader::CheckForErrors(GLuint shader, GLuint type) {
	int params=0;
	if (type == GL_COMPILE_STATUS)	glGetShaderiv(shader, type, &params);
	if (type == GL_LINK_STATUS)	glGetProgramiv(shader, type, &params);
	if (GL_TRUE != params) {
		//printf("ERROR: GL shader index %i (%s) did not compile\n", shader, filename);

		int max_length = 2048;
		int actual_length = 0;
		char shader_log[2048];
		if (type == GL_COMPILE_STATUS)	glGetShaderInfoLog(shader, max_length, &actual_length, shader_log);
		if (type == GL_LINK_STATUS)	glGetProgramInfoLog(shader, max_length, &actual_length, shader_log);
		printf("Shader %s info log for GL index %u:\n%s\n", (type==GL_LINK_STATUS)?"link":"compile", shader, shader_log);
		return GL_TRUE;

		//return false; // or exit or something
	}
	if (type == GL_COMPILE_STATUS) printf("Compile %i okay\n", shader);
	else if (type == GL_LINK_STATUS) printf("Link %i okay\n", shader);
	else return GL_TRUE;
	
	
	return GL_FALSE;

	
}