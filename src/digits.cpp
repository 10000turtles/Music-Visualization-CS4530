// g++ digits.cpp -lglut -lGLEW -lGLU -lGL -lSDL2 -lglfw -I
// /home/turtles/Documents/Code/School/eigen/src -o out.exe

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

using namespace std;

float mapp(float value, float a, float b, float c, float d);
GLuint LoadShaders(const char *vertex_file_path,
                   const char *fragment_file_path);

struct point {
  double x;
  double y;
  point(double x1, double y1) {
    x = x1;
    y = y1;
  }

  int value;
};
double gen_rand_float() {
  static std::random_device rd;
  static std::mt19937 engine(rd());
  static std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(engine);
}

void OpenGLStuffDisplayDigit(vector<point> points,
                             vector<double> colorsOfPoints) {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "ERROR: Failed to initialize GLFW" << std::endl;
    exit(1);
  }

  // We will ask it to specifically open an OpenGL 3.2 context
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a GLFW window
  int size = 1000;
  GLFWwindow *window = glfwCreateWindow(size, size, "ACG HW0 IFS", NULL, NULL);
  if (!window) {
    std::cerr << "ERROR: Failed to open GLFW window" << std::endl;
    glfwTerminate();
    exit(1);
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "ERROR: Failed to initialize GLEW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  std::cout << "-------------------------------------------------------"
            << std::endl;
  std::cout << "OpenGL Version: " << (char *)glGetString(GL_VERSION) << '\n';
  std::cout << "-------------------------------------------------------"
            << std::endl;

  glClearColor(0.0f, 1.0f, 1.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  int dim = 3;
  float borderProportion = 0.99;

  GLfloat *vertices = new GLfloat[points.size() * dim];
  GLfloat *colors = new GLfloat[points.size() * dim];

  for (int i = 0; i < points.size() * dim; i += dim) {
    vertices[i] =
        mapp(points[i / dim].x, 0, 1, -borderProportion, borderProportion);
    vertices[i + 1] =
        mapp(points[i / dim].y, 0, 1, -borderProportion, borderProportion);
    vertices[i + 2] = 0;
  }
  for (int i = 0; i < colorsOfPoints.size(); i++) {
    for (int j = 0; j < 6; j++) {
      colors[i * dim * 6 + j * dim] = colorsOfPoints[i] / 256.0;
      colors[i * dim * 6 + j * dim + 1] = colorsOfPoints[i] / 256.0;
      colors[i * dim * 6 + j * dim + 2] = colorsOfPoints[i] / 256.0;
    }
  }

  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * points.size() * dim, vertices,
               GL_STATIC_DRAW);

  GLuint colorbuffer;
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * points.size() * dim, colors,
               GL_STATIC_DRAW);

  // GLuint vertexbuffer2;
  // glGenBuffers(1, &vertexbuffer2);
  // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, lines2, GL_STATIC_DRAW);

  // GLuint colorbuffer2;
  // glGenBuffers(1, &colorbuffer2);
  // glBindBuffer(GL_ARRAY_BUFFER, colorbuffer2);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 18, lineColors2,
  // GL_STATIC_DRAW);

  GLuint programID = LoadShaders("../src/SimpleVertexShader.vertexshader",
                                 "../src/SimpleFragmentShader.fragmentshader");
  glEnable(GL_POINT_SMOOTH);
  glPointSize(6.0f);

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 1st attribute buffer : vertices
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glUseProgram(programID);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Draw the triangle !
    glDrawArrays(
        GL_TRIANGLES, 0,
        points
            .size()); // Starting from vertex 0; 3 vertices total -> 1 triangle

    // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // glBindBuffer(GL_ARRAY_BUFFER, colorbuffer2);
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    // glDrawArrays(GL_LINES, 0, 6);
    // glDisableVertexAttribArray(2);

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Close OpenGL window and terminate GLFW
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

void OpenGLStuffDisplayScatterPlot(vector<pair<int, point>> c, double *lines) {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "ERROR: Failed to initialize GLFW" << std::endl;
    exit(1);
  }

  // We will ask it to specifically open an OpenGL 3.2 context
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a GLFW window
  int size = 1000;
  GLFWwindow *window = glfwCreateWindow(size, size, "ACG HW0 IFS", NULL, NULL);
  if (!window) {
    std::cerr << "ERROR: Failed to open GLFW window" << std::endl;
    glfwTerminate();
    exit(1);
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "ERROR: Failed to initialize GLEW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  std::cout << "-------------------------------------------------------"
            << std::endl;
  std::cout << "OpenGL Version: " << (char *)glGetString(GL_VERSION) << '\n';
  std::cout << "-------------------------------------------------------"
            << std::endl;

  glClearColor(0.0f, 1.0f, 1.0f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  int dim = 3;
  float borderProportion = 0.99;

  GLfloat *vertices = new GLfloat[c.size() * dim];
  GLfloat *colors = new GLfloat[c.size() * dim];

  GLfloat *lines2 = new GLfloat[6];
  GLfloat *lineColors2 = new GLfloat[6];

  for (int i = 0; i < c.size() * dim; i += dim) {
    vertices[i] =
        mapp(c[i / dim].second.x, 0, .1, -borderProportion, borderProportion);
    vertices[i + 1] =
        mapp(c[i / dim].second.y, 0, .1, -borderProportion, borderProportion);
    vertices[i + 2] = 0;
    colors[i] = (c[i / dim].first - 1) / 4.0;
    colors[i + 1] = 0;
    colors[i + 2] = 1 - ((c[i / dim].first - 1) / 4.0);
  }

  for (int i = 0; i < 6; i++) {
    lines2[i] = mapp(lines[i], 0, .1, -borderProportion, borderProportion);
  }

  lineColors2[0] = 0.0;
  lineColors2[1] = 1.0;
  lineColors2[2] = 0.0;
  lineColors2[3] = 0.0;
  lineColors2[4] = 1.0;
  lineColors2[5] = 0.0;

  GLuint vertexbuffer;
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * c.size() * dim, vertices,
               GL_STATIC_DRAW);

  GLuint colorbuffer;
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * c.size() * dim, colors,
               GL_STATIC_DRAW);

  GLuint vertexbuffer2;
  glGenBuffers(1, &vertexbuffer2);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, lines2, GL_STATIC_DRAW);

  GLuint colorbuffer2;
  glGenBuffers(1, &colorbuffer2);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer2);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6, lineColors2, GL_STATIC_DRAW);

  GLuint programID = LoadShaders("../src/SimpleVertexShader.vertexshader",
                                 "../src/SimpleFragmentShader.fragmentshader");
  glEnable(GL_POINT_SMOOTH);
  glPointSize(6.0f);

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 1st attribute buffer : vertices
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glUseProgram(programID);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0, dim, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Draw the triangle !
    glDrawArrays(
        GL_POINTS, 0,
        c.size()); // Starting from vertex 0; 3 vertices total -> 1 triangle

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer2);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glDrawArrays(GL_LINES, 0, 6);
    glDisableVertexAttribArray(2);

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Close OpenGL window and terminate GLFW
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

GLuint LoadShaders(const char *vertex_file_path,
                   const char *fragment_file_path) {
  // Create the shaders
  GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  // Read the Vertex Shader code from the file
  std::string VertexShaderCode;
  std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
  if (VertexShaderStream.is_open()) {
    std::stringstream sstr;
    sstr << VertexShaderStream.rdbuf();
    VertexShaderCode = sstr.str();
    VertexShaderStream.close();
  } else {
    printf("Impossible to open %s. Are you in the right directory ? Don't "
           "forget to read the FAQ !\n",
           vertex_file_path);
    getchar();
    return 0;
  }

  // Read the Fragment Shader code from the file
  std::string FragmentShaderCode;
  std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
  if (FragmentShaderStream.is_open()) {
    std::stringstream sstr;
    sstr << FragmentShaderStream.rdbuf();
    FragmentShaderCode = sstr.str();
    FragmentShaderStream.close();
  }

  GLint Result = GL_FALSE;
  int InfoLogLength;

  // Compile Vertex Shader
  printf("Compiling shader : %s\n", vertex_file_path);
  char const *VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
  glCompileShader(VertexShaderID);

  // Check Vertex Shader
  glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0) {
    std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL,
                       &VertexShaderErrorMessage[0]);
    printf("%s\n", &VertexShaderErrorMessage[0]);
  }

  // Compile Fragment Shader
  printf("Compiling shader : %s\n", fragment_file_path);
  char const *FragmentSourcePointer = FragmentShaderCode.c_str();
  glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
  glCompileShader(FragmentShaderID);

  // Check Fragment Shader
  glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0) {
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL,
                       &FragmentShaderErrorMessage[0]);
    printf("%s\n", &FragmentShaderErrorMessage[0]);
  }

  // Link the program
  printf("Linking program\n");
  GLuint ProgramID = glCreateProgram();
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);

  // Check the program
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0) {
    std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL,
                        &ProgramErrorMessage[0]);
    printf("%s\n", &ProgramErrorMessage[0]);
  }

  glDetachShader(ProgramID, VertexShaderID);
  glDetachShader(ProgramID, FragmentShaderID);

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);

  return ProgramID;
}

float mapp(float value, float a, float b, float c, float d) {
  return c + (value - a) * (d - c) / (b - a);
}

double func(vector<double> w, vector<pair<int, point>> c) {
  double temp = 0;
  for (int i = 0; i < c.size(); i++) {
    double wTx = w[0] + w[1] * c[i].second.x + w[2] * c[i].second.y;
    temp += log(1 + exp(-c[i].second.value * wTx));
  }
  temp *= (1.0 / c.size());
  return temp;
}

vector<double> gradf(vector<double> w, vector<pair<int, point>> c) {
  vector<double> temp = {0, 0, 0};
  for (int i = 0; i < c.size(); i++) {
    double wTx = w[0] + w[1] * c[i].second.x + w[2] * c[i].second.y;
    temp[0] += c[i].second.value * (1 / (1 + exp(c[i].second.value * wTx)));
    temp[1] += c[i].second.value * c[i].second.x *
               (1 / (1 + exp(c[i].second.value * wTx)));
    temp[2] += c[i].second.value * c[i].second.y *
               (1 / (1 + exp(c[i].second.value * wTx)));
  }
  temp[0] *= -(1.0 / c.size());
  temp[1] *= -(1.0 / c.size());
  temp[2] *= -(1.0 / c.size());
  return temp;
}

int main() {
  ifstream f("../src/ZipDigits.train");
  string temp;
  int num;
  int displayDigit = 3;

  vector<point> vertexData;
  vector<double> greyScaleData;

  vector<pair<int, vector<double>>> digits;
  while (f >> temp) {
    num = stoi(temp);
    vector<double> tmparr;
    for (int i = 0; i < 256; i++) {
      f >> temp;
      tmparr.push_back((stod(temp) + 1) * 128);
    }
    if (num == 1 || num == 5) {
      digits.push_back(make_pair(num, tmparr));
    }
  }

  ifstream fff("../src/ZipDigits.test");
  vector<pair<int, vector<double>>> testDigits;
  while (fff >> temp) {
    num = stoi(temp);
    vector<double> tmparr;
    for (int i = 0; i < 256; i++) {
      fff >> temp;
      tmparr.push_back((stod(temp) + 1) * 128);
    }
    if (num == 1 || num == 5) {
      testDigits.push_back(make_pair(num, tmparr));
    }
  }

  int def = 16;

  vector<pair<int, point>> classifications;
  vector<pair<int, point>> testData;
  int order = 3;

  double min5b = 0;
  int min5bindex = 0;
  for (int i = 0; i < digits.size(); i++) {
    double B = 0;
    for (int j = 0; j < digits[i].second.size(); j++) {
      B += digits[i].second[j] / 256;
    }
    B /= def * def;

    double S = 0;
    for (int j = 0; j < def / 2; j++) {
      for (int k = 0; k < def; k++) {
        double t = (digits[i].second[j * def + k] -
                    digits[i].second[(def - 1 - j) * def + k]) /
                   256;
        S += t > 0 ? t : -t;
      }
    }
    S /= def * def / 2;

    if (digits[i].first == 1 && S > min5b) {
      min5b = S;
      min5bindex = i;
      cout << min5bindex << ": " << min5b << endl;
    }

    classifications.push_back(
        make_pair(digits[i].first, point(pow(B, order), pow(S, order))));
    classifications[i].second.value = (digits[i].first == 1) ? -1 : 1;
  }

  for (int i = 0; i < testDigits.size(); i++) {
    double B = 0;
    for (int j = 0; j < testDigits[i].second.size(); j++) {
      B += testDigits[i].second[j] / 256;
    }
    B /= def * def;

    double S = 0;
    for (int j = 0; j < def / 2; j++) {
      for (int k = 0; k < def; k++) {
        double t = (testDigits[i].second[j * def + k] -
                    testDigits[i].second[(def - 1 - j) * def + k]) /
                   256;
        S += t > 0 ? t : -t;
      }
    }
    S /= def * def / 2;

    if (testDigits[i].first == 1 && S > min5b) {
      min5b = S;
      min5bindex = i;
      cout << min5bindex << ": " << min5b << endl;
    }

    testData.push_back(
        make_pair(testDigits[i].first, point(pow(B, order), pow(S, order))));
    testData[i].second.value = (testDigits[i].first == 1) ? -1 : 1;
  }

  displayDigit = min5bindex;
  for (int i = 0; i < def; i++) {
    for (int j = 0; j < def; j++) {
      double x = mapp(j, 0, def, 0, 1);
      double y = mapp(def - 1 - i, 0, def, 0, 1);
      double d = mapp(1, 0, def, 0, 1);
      vertexData.push_back(point(x, y));
      vertexData.push_back(point(x + d, y));
      vertexData.push_back(point(x + d, y + d));
      vertexData.push_back(point(x, y));
      vertexData.push_back(point(x + d, y + d));
      vertexData.push_back(point(x, y + d));
      greyScaleData.push_back(digits[displayDigit].second[i * def + j]);
    }
  }

  double learningRate = .01;
  int n = 500000;

  vector<double> w = {gen_rand_float(), gen_rand_float(), gen_rand_float()};

  for (int i = 0; i < n; i++) {
    vector<double> g_t = gradf(w, classifications);

    w[0] = w[0] - learningRate * g_t[0];
    w[1] = w[1] - learningRate * g_t[1];
    w[2] = w[2] - learningRate * g_t[2];
  }

  double *lines = new double[6];

  lines[0] = 0;
  lines[1] = -w[0] / w[2];
  lines[2] = 0;
  lines[3] = 1;
  lines[4] = (-w[0] - w[1]) / w[2];
  lines[5] = 0;

  std::cout << "w: " << w[0] << " " << w[1] << " " << w[2] << endl;
  std::cout << "w (line): y = " << -w[1] / w[2] << "x + " << -w[0] / w[2]
            << endl;

  cout << "E_in: " << func(w, classifications)
       << " E_test: " << func(w, testData) << endl;

  double delta = 0.05;

  cout << "E_out < E_in + "
       << sqrt(log(2 * classifications.size() / delta) /
               classifications.size() / 2)
       << endl;
  cout << "E_out < test + " << sqrt(log(2 / delta) / testData.size() / 2)
       << endl;

  OpenGLStuffDisplayScatterPlot(testData, lines);
  // OpenGLStuffDisplayDigit(vertexData, greyScaleData);
}