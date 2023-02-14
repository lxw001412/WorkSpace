#include "CRender.h"
#include <string>
#include "CLog.h"
#include <sstream>
#define GET_STR(x) #x

//顶点shader
const char *vString = GET_STR(
	attribute vec4 vertexIn;
attribute vec2 textureIn;
varying vec2 textureOut;
void main(void)
{
	gl_Position = vertexIn;
	textureOut = textureIn;
});

//片元shader
const char *tString = GET_STR(
	varying vec2 textureOut;
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;
void main(void)
{
	vec3 yuv;
	vec3 rgb;
	yuv.x = texture2D(tex_y, textureOut).r;
	yuv.y = texture2D(tex_u, textureOut).r - 0.5;
	yuv.z = texture2D(tex_v, textureOut).r - 0.5;
	rgb = mat3(1.0, 1.0, 1.0,
		0.0, -0.39465, 2.03211,
		1.13983, -0.58060, 0.0) * yuv;
	gl_FragColor = vec4(rgb, 1.0);
});

void YUVRender::LoadPicture()
{
	glGenTextures(3, texIndexarray);//生成三个纹理索引

	glBindTexture(GL_TEXTURE_2D, texIndexarray[0]);
	//为bind的纹理设置环绕，过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, texIndexarray[1]);
	//为bind的纹理设置环绕，过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, texIndexarray[2]);
	//为bind的纹理设置环绕，过滤方式
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	//使用着色器程序，返回采样器的序号
	glUseProgram(shaderProgram);//该语句必须要有；安装 指定着色器程序
	texUniformY = glGetUniformLocation(shaderProgram, "tex_y");
	texUniformU = glGetUniformLocation(shaderProgram, "tex_u");
	texUniformV = glGetUniformLocation(shaderProgram, "tex_v");

	void* uptr = m_ptr + m_width * m_height;
	void* vptr = m_ptr + m_width * m_height * 5 / 4;

	//---------------------------------------------------------------------------
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texIndexarray[0]);// texindexarray[0] =1
	//使用GL_red表示单通道，glfw3里边没有YUV那个GL属性；
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, m_ptr);
	glUniform1i(texUniformY, 0);                //通过 glUniform1i 的设置，保证每个 uniform 采样器对应着正确的纹理单元;注意这里不能用tesindexarray[0];


	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texIndexarray[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_width / 2, m_height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, uptr);

	glUniform1i(texUniformU, 1);


	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texIndexarray[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_width / 2, m_height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, vptr);
	glUniform1i(texUniformV, 2);

	glUseProgram(0);
}


void YUVRender::Render()
{
	glBindVertexArray(VAO);
	glUseProgram(shaderProgram);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glUseProgram(0);
	glBindVertexArray(0);
}

void YUVRender::Initmodule()
{
	//做个一模型;正方形；映射了顶点坐标和纹理坐标的对应关系
	float vertexs[] = {
		//顶点坐标-------纹理坐标(屏幕坐标翻转)
		1.0f,  1.0f, 0.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,  1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f,  0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f,  0.0f, 0.0f


	};
	//一个正方形是由两个三角形得来的；记录顶点的索引顺序
	unsigned int indexs[] = {
		0,1,3,
		1,2,3,
	};

	//做VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//做VBO

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//创建显存空间
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexs), vertexs, GL_STATIC_DRAW);

	//设置索引缓冲
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexs), indexs, GL_STATIC_DRAW);    //加载纹理图片,生成纹理

	LoadPicture();

	//设置第0个锚点,3个点，不需要归一化，跨度5个float可以读下一个点
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	//打开顶点
	glEnableVertexAttribArray(0);
	//纹理属性设置,纹理在第一个锚点上（指定顶点数据）
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	//打开纹理
	glEnableVertexAttribArray(1);

	//解除绑定VBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//解绑VAO
	glBindVertexArray(0);
}

void YUVRender::Initshader()
{
	//shader 编译连接
	unsigned int vertexID = 0, fragID = 0;
	char infoLog[512];//存储错误信息
	int  successflag = 0;
	vertexID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexID, 1, &vString, NULL);
	glCompileShader(vertexID);
	//获取编译是否成功
	glGetShaderiv(vertexID, GL_COMPILE_STATUS, &successflag);
	if (!successflag)
	{
		glGetShaderInfoLog(vertexID, 512, NULL, infoLog);
		std::string errstr(infoLog);
	}
	//frag
	fragID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragID, 1, &tString, NULL);
	glCompileShader(fragID);
	//获取编译是否成功
	glGetShaderiv(fragID, GL_COMPILE_STATUS, &successflag);
	if (!successflag)
	{
		glGetShaderInfoLog(fragID, 512, NULL, infoLog);
		std::string errstr(infoLog);
	}
	//链接
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexID);
	glAttachShader(shaderProgram, fragID);

	glBindAttribLocation(shaderProgram, 0, "vertexIn");
	glBindAttribLocation(shaderProgram, 1, "textureIn");

	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &successflag);
	if (!successflag)
	{
		glGetShaderInfoLog(shaderProgram, 512, NULL, infoLog);
		std::string errstr(infoLog);
	}

	//编译完成后，可以把中间的步骤程序删除
	glDeleteShader(vertexID);
	glDeleteShader(fragID);
}
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		//将窗口设置为关闭，跳出循环
		glfwSetWindowShouldClose(window, true);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

YUVRender::~YUVRender()
{
	if (m_ptr)
	{
		delete m_ptr;
		m_ptr = nullptr;
	}
	LOG_I("RENDER EXIT");
}

void YUVRender::Init(unsigned char * data, int w, int h)
{
	m_width = w;
	m_height = h;
	m_ptr = new unsigned char[w * h * 3 / 2];
	memcpy(m_ptr, data, w * h * 3 / 2);
	Start();
}


void YUVRender::ThreadWorker()
{
	if (!m_ptr)return;
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	int screenwidth = GetSystemMetrics(SM_CXSCREEN);
	int screenheight = GetSystemMetrics(SM_CYSCREEN);

	//glfw创建窗口
	double Scale = 0.0;
	if (screenwidth >= screenheight)
	{
		Scale = (double)screenwidth / (double)screenheight;
	}
	else
	{
		Scale = (double)screenheight / (double)screenwidth;
	}
	Scale = Scale * ((double)3 / (double)4);
	m_window = glfwCreateWindow((m_width * Scale),(m_height * Scale), "CamerasMoudle", NULL, NULL);
	if (m_window == NULL)
	{
		printf("创建窗口失败");
		//终止
		glfwTerminate();
		return;
	}

	glfwSetWindowPos(m_window, screenwidth / 2 - (m_width * Scale) / 2, screenheight / 2 - (m_height * Scale) / 2);
	//显示窗口
	glfwMakeContextCurrent(m_window);

	//设置回调，当窗口大小调整后将调用该回调函数
	glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);

	// glad初始化
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("加载失败");
		return;
	}
	Initshader();//先编译着色器
	Initmodule();
	// 使用循环达到循环渲染效果
	while (!glfwWindowShouldClose(m_window) && !m_Stop)
	{
		//自定义输入事件
		processInput(m_window);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		Render();
		//交互缓冲区，否则显示空白
		glfwSwapBuffers(m_window);
		//输入输出事件,否则无法对窗口进行交互
		glfwPollEvents();
	}
	//终止渲染 关闭并清理glfw本地资源
	glfwTerminate();
	return;

}
