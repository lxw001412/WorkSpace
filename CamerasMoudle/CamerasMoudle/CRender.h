#pragma once
#include "CThread.h"
#include <process.h>
#include <stdio.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"


class YUVRender : public CThread
{
public:
	YUVRender() {
		m_width = 0;
		m_height = 0;
		m_ptr = nullptr;
		m_window = nullptr;
		m_Stop = false;
		m_ptry = nullptr;
		m_ptru = nullptr;
		m_ptry = nullptr;
	}
	~YUVRender();
	void Stop() { m_Stop = true; }
	GLFWwindow* GetRenderWindow() { return m_window; }
	void Init(unsigned char* data, int w, int h);
protected:
	void Render();
	void Initshader();
	void Initmodule();
	void LoadPicture();
	virtual void ThreadWorker() override;
private:
	int m_width;
	int m_height;
	unsigned char* m_ptr;

	unsigned char* m_ptry;
	unsigned char* m_ptru;
	unsigned char* m_ptrv;

	GLuint VBO = 0;
	GLuint VAO = 0;
	GLuint EBO = 0;
	int shaderProgram = 0;

	GLuint texIndexarray[3];
	GLuint texUniformY = 99;
	GLuint texUniformU = 99;
	GLuint texUniformV = 99;
	GLFWwindow* m_window;
	bool   m_Stop;
};
