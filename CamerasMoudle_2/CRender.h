#pragma once
#include "CThread.h"
struct GLFWwindow;
typedef unsigned int GLuint;

class YUVRender : public CThread
{
public:
	YUVRender();
	~YUVRender();
	YUVRender(const YUVRender&) = delete;
	YUVRender& operator = (const YUVRender&) = delete;
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
	bool   m_Stop;
	int    m_width;
	int    m_height;
	unsigned char* m_ptr;

	GLuint VBO = 0;
	GLuint VAO = 0;
	GLuint EBO = 0;
	int shaderProgram = 0;

	GLuint texIndexarray[3];
	GLuint texUniformY = 99;
	GLuint texUniformU = 99;
	GLuint texUniformV = 99;
	GLFWwindow* m_window;
};
