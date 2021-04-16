#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <stdio.h>
#include <stdlib.h>

#include "Binasc.cpp"
#include "MidiEvent.cpp"
#include "MidiEventList.cpp"
#include "MidiFile.cpp"
#include "MidiMessage.cpp"
#include "Options.cpp"

#include <chrono>
#include <experimental/filesystem>
#include <map>
#include <set>
#include <vector>

using namespace std;
using namespace glm;
using namespace smf;

namespace fs = experimental::filesystem;
typedef std::chrono::high_resolution_clock Clock;

int CIRCLE_DEFINITION = 100;
int DIM = 3;

template <class T> vector<T> subVec(vector<T> data, int a, int b);
template <class T> vector<T> concat(T thing, vector<T> list);
template <class T> vector<T> concat(vector<T> list, vector<T> addedList);
template <class T> string strVec(vector<T> data);
float mapp(float value, float a, float b, float c, float d);

double getXPos(vector<int> wk, int f) {
  for (int i = 0; i < wk.size(); i++) {
    if (f == wk[i]) {
      return i;
    }
  }
  return getXPos(wk, f - 1) + 0.5;
}

enum ShapeType { SQUARE, CIRCLE, STAR };

class Args {
public:
  string configPath;
  string song;
  string readPath;

  bool record;

  Args() {}
  Args(string c) {
    configPath = c;
    record = false;

    ifstream f(configPath);
    string temp;

    while (f >> temp) {
      if (temp == string("-song")) {
        f >> temp;
        song = string(temp);
      }
      if (temp == string("-readPath")) {
        f >> temp;
        readPath = string(temp);
      }
      if (temp == string("-record")) {
        f >> temp;
        if (temp == string("true")) {
          record = true;
        }
      }
    }
    fs::directory_iterator entry;
    if (song != string("")) {
      entry = fs::directory_iterator(readPath + song);
    } else {
      cout << "Error: No song name passed" << endl;
    }
    if (entry == end(entry)) {
      cout << "Error: No songs in folder/no folder" << endl;
    }
  }
};

class Note {
public:
  bool onOff;
  double time;
  double intensity;

  Note() {}
  Note(bool o, double t, double i) {
    onOff = o;
    time = t;
    intensity = i;
  }
};

class ShapeData {
public:
  int note;

  vector<vec3> mesh;

  vec3 center;
  vec3 offColor;
  vec3 onColor;

  // Open GL stuff
  GLenum drawtype;
  GLfloat *vertices;
  GLfloat *colors;
  GLuint vertexbuffer;
  GLuint colorbuffer;

  vector<Note> notes;

  ShapeData() {}
  ShapeData(int n, vector<vec3> m, GLenum d, vec3 cen, vec3 col,
            vector<Note> no) {
    note = n;
    mesh = m;
    drawtype = d;
    center = cen;
    onColor = col;
    offColor.x = col.x * 0.2;
    offColor.y = col.y * 0.2;
    offColor.z = col.z * 0.2;
    notes = no;
  }

  void setup() {

    float borderProportion = 0.99;

    vertices = new GLfloat[mesh.size() * DIM];
    colors = new GLfloat[mesh.size() * DIM];

    for (int i = 0; i < mesh.size() * DIM; i += DIM) {
      vertices[i] =
          mapp(mesh[i / DIM].x, -1, 1, -borderProportion, borderProportion);

      vertices[i] = mapp(vertices[i], -1, 1, -1, 0);
      vertices[i + 1] =
          mapp(mesh[i / DIM].y, -1, 1, -borderProportion, borderProportion);
      vertices[i + 1] = mapp(vertices[i + 1], -1, 1, 0, 1);
      vertices[i + 2] = 0;
      colors[i] = offColor.x;
      colors[i + 1] = offColor.y;
      colors[i + 2] = offColor.z;
    }

    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.size() * DIM, vertices,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.size() * DIM, colors,
                 GL_STATIC_DRAW);
  }
  void draw() {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.size() * DIM, vertices,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, DIM, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * mesh.size() * DIM, colors,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1, DIM, GL_FLOAT, GL_FALSE, 0, (void *)0);

    // Draw the triangle !
    glDrawArrays(
        drawtype, 0,
        mesh.size()); // Starting from vertex 0; 3 vertices total -> 1 triangle
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
  }
  void on() {
    for (int i = 0; i < mesh.size() * DIM; i += DIM) {
      colors[i] = onColor.x;
      colors[i + 1] = onColor.y;
      colors[i + 2] = onColor.z;
    }
  }
  void off() {
    for (int i = 0; i < mesh.size() * DIM; i += DIM) {
      colors[i] = offColor.x;
      colors[i + 1] = offColor.y;
      colors[i + 2] = offColor.z;
    }
  }
};

class Timer {
public:
  void start() {
    m_StartTime = std::chrono::system_clock::now();
    m_bRunning = true;
  }

  void stop() {
    m_EndTime = std::chrono::system_clock::now();
    m_bRunning = false;
  }

  double elapsedMilliseconds() {
    std::chrono::time_point<std::chrono::system_clock> endTime;

    if (m_bRunning) {
      endTime = std::chrono::system_clock::now();
    } else {
      endTime = m_EndTime;
    }

    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime -
                                                                 m_StartTime)
        .count();
  }

  double elapsedSeconds() { return elapsedMilliseconds() / 1000.0; }

private:
  std::chrono::time_point<std::chrono::system_clock> m_StartTime;
  std::chrono::time_point<std::chrono::system_clock> m_EndTime;
  bool m_bRunning = false;
};

vector<vec3> makeMesh(ShapeType shape, vec3 center, double scale) {
  if (shape == CIRCLE) {
    vector<vec3> mesh;
    for (int i = 0; i < CIRCLE_DEFINITION; i++) {

      mesh.push_back(vec3(cos(i * 2 * M_PI / CIRCLE_DEFINITION) * scale,
                          sin(i * 2 * M_PI / CIRCLE_DEFINITION) * scale, 0) +
                     center);
    }
    return mesh;
  }
  return {{0, 0, 0}};
}

vector<vec3> pianoMesh(ShapeType shape, int key, int minKey, int maxKey) {
  vec3 center = {0, 0, 0};
  double leftBorder = -0.7;
  double rightBorder = 0.7;
  vector<int> whiteKeys = {21, 23, 24,  26,  28,  29,  31,  33, 35, 36, 38,
                           40, 41, 43,  45,  47,  48,  50,  52, 53, 55, 57,
                           59, 60, 62,  64,  65,  67,  69,  71, 72, 74, 76,
                           77, 79, 81,  83,  84,  86,  88,  89, 91, 93, 95,
                           96, 98, 100, 101, 103, 105, 107, 108};

  double scale = 0.9 * (rightBorder - leftBorder) /
                 (getXPos(whiteKeys, maxKey) - getXPos(whiteKeys, minKey) + 1);

  // vector<int> blackKeys = {22, 25, 27, 30, 32, 34, 37, 39, 42, 44,  46, 49,
  //                          51, 54, 56, 58, 61, 63, 66, 68, 70, 73,  75, 78,
  //                          80, 82, 85, 87, 90, 92, 94, 97, 99, 102, 104,
  //                          106};

  vec3 wbadj = {mapp(getXPos(whiteKeys, key), getXPos(whiteKeys, minKey),
                     getXPos(whiteKeys, maxKey), leftBorder, rightBorder),
                find(whiteKeys.begin(), whiteKeys.end(), key) == whiteKeys.end()
                    ? scale
                    : 0,
                0};

  vector<vec3> mesh = makeMesh(shape, center + wbadj, scale / 2);

  return mesh;
}

class Instrument {
public:
  vector<ShapeData> shapes;
  map<int, vector<Note>> notes;

  Instrument(Args a) {
    MidiFile file;
    file.read(a.readPath + a.song + "/Melody.mid");

    int minKey = 108;
    int maxKey = 21;

    for (int i = 3; i < file.getEventCount(0); i++) {
      notes[file.getEvent(0, i).getP1()].push_back(
          Note(file.getEvent(0, i).getP0() == 144, file.getTimeInSeconds(0, i),
               file.getEvent(0, i).getP2()));
      if (file.getEvent(0, i).getP1() < minKey) {
        minKey = file.getEvent(0, i).getP1();
      }
      if (file.getEvent(0, i).getP1() > maxKey) {
        maxKey = file.getEvent(0, i).getP1();
      }
    }
    for (int i = minKey; i < maxKey; i++) {
      notes[i].push_back(Note(true, 0, 0));
    }

    for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
         i++) {
      shapes.push_back(
          ShapeData((*i).first, pianoMesh(CIRCLE, (*i).first, minKey, maxKey),
                    GL_LINE_LOOP, vec3(0, 0, 0), vec3(0, 1, 0), (*i).second));
    }
  }

  void update(double pTime, double cTime) {
    for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
         i++) {
      for (int j = 0; j < (*i).second.size(); j++) {
        if ((*i).second[j].time < cTime && (*i).second[j].time > pTime) {
          for (int k = 0; k < shapes.size(); k++) {
            if (shapes[k].note == (*i).first) {
              if ((*i).second[j].onOff) {
                shapes[k].on();
              } else {
                shapes[k].off();
              }
              break;
            }
          }
        }
      }
    }
  }

  void draw() {
    for (int i = 0; i < shapes.size(); i++) {
      shapes[i].draw();
    }
  }
  void setup() {
    for (int i = 0; i < shapes.size(); i++) {
      shapes[i].setup();
    }
  }

  void on(int note) {
    ShapeData *temp;
    for (int i = 0; i < shapes.size(); i++) {
      if (note == shapes[i].note) {
        temp = &shapes[i];
        break;
      }
    }
    temp->on();
  }
  void off(int note) {
    ShapeData *temp;
    for (int i = 0; i < shapes.size(); i++) {
      if (note == shapes[i].note) {
        temp = &shapes[i];
        break;
      }
    }
    temp->off();
  }
};

float mapp(float value, float a, float b, float c, float d) {
  return c + (value - a) * (d - c) / (b - a);
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

void OpenGLStuff(vector<Instrument> instruments, Args a) {
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

  for (int i = 0; i < instruments.size(); i++) {
    instruments[i].setup();
  }

  GLuint programID = LoadShaders("../src/SimpleVertexShader.vertexshader",
                                 "../src/SimpleFragmentShader.fragmentshader");
  glEnable(GL_POINT_SMOOTH);
  glPointSize(3.0f);

  Timer timer;
  timer.start();

  double currentTime = timer.elapsedSeconds();
  double prevTime = timer.elapsedSeconds();

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 1st attribute buffer : vertices
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glUseProgram(programID);

    currentTime = timer.elapsedSeconds();

    for (int i = 0; i < instruments.size(); i++) {
      instruments[i].update(prevTime, currentTime);
      instruments[i].draw();
    }

    prevTime = currentTime;

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Close OpenGL window and terminate GLFW
  glfwDestroyWindow(window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

int main(int argc, char *args[]) {
  vector<Instrument> inst;
  Args a(args[1]);
  inst.push_back(Instrument(a));
  OpenGLStuff(inst, a);
}

template <class T> vector<T> subVec(vector<T> data, int a, int b) {
  vector<T> temp;
  for (int i = a; i < b; i++)
    temp.push_back(data[i]);
  return temp;
}

template <class T> vector<T> concat(T thing, vector<T> list) {
  vector<T> temp;
  temp.push_back(thing);
  for (int i = 0; i < list.size(); i++)
    temp.push_back(list[i]);
  return temp;
}

template <class T> vector<T> concat(vector<T> list, vector<T> addedList) {
  vector<T> temp = list;
  for (int i = 0; i < addedList.size(); i++)
    temp.push_back(addedList[i]);
  return temp;
}

template <class T> vector<T> toVector(set<T> s) {
  vector<T> temp;
  for (typename set<T>::iterator i = s.begin(); i != s.end(); i++) {
    temp = concat((*i), temp);
  }
  return temp;
}