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

std::string output_of_system_command(const char *cmd) {
  char buffer[128];
  std::string result = "";
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer, 128, pipe.get()) != NULL)
      result += buffer;
  }
  return result;
}
bool windowExists(std::string window_name) {
  std::string command = "xwininfo -name \"" + window_name + "\"";
  std::string output = output_of_system_command(command.c_str());
  // to check if the window exists we look to see if we are given an xwininfo
  // error. TODO: I would like to find a better way to do this.
  if (output.find("xwininfo: error:") != std::string::npos) {
    return false;
  } else {
    return true;
  }
}

bool screenshot(std::string window_name, std::string screenshot_name) {
  if (windowExists(window_name)) {
    // if the window hasn't crashed, bring it into focus and screenshot it
    std::string command =
        //"wmctrl -R " + window_name + " &&
        " scrot " + screenshot_name + " -u";
    std::system(command.c_str());
    return true;
  } else {
    std::cout << "Attempted to screenshot a closed window." << std::endl;
    return false;
  }
}

enum ShapeType { DOT, CIRCLE };
enum InstrumentType { PIANO, BASS, DRUMS, SOLO, BACKGROUND };

class Args {
public:
  string configPath;

  string pianoPath;
  string bassPath;
  string drumsPath;
  string soloPath;
  string backgroundPath;

  int pianoOffset;
  int bassOffset;
  int drumsOffset;
  int soloOffset;
  int backgroundOffset;

  bool record;

  Args() {}
  Args(string c) {
    configPath = c;
    record = false;

    ifstream f(configPath);
    string temp;

    while (f >> temp) {

      if (temp == string("-pianoPath")) {
        f >> temp;
        pianoPath = string(temp);
      }
      if (temp == string("-bassPath")) {
        f >> temp;
        bassPath = string(temp);
      }
      if (temp == string("-drumsPath")) {
        f >> temp;
        drumsPath = string(temp);
      }
      if (temp == string("-soloPath")) {
        f >> temp;
        soloPath = string(temp);
      }
      if (temp == string("-backgroundPath")) {
        f >> temp;
        backgroundPath = string(temp);
      }
      if (temp == string("-pianoOffset")) {
        f >> temp;
        pianoOffset = stoi(temp);
      }
      if (temp == string("-bassOffset")) {
        f >> temp;
        bassOffset = stoi(temp);
      }
      if (temp == string("-drumsOffset")) {
        f >> temp;
        drumsOffset = stoi(temp);
      }
      if (temp == string("-soloOffset")) {
        f >> temp;
        soloOffset = stoi(temp);
      }
      if (temp == string("-backgroundOffset")) {
        f >> temp;
        backgroundOffset = stoi(temp);
      }
      if (temp == string("-record")) {
        f >> temp;
        if (temp == string("true")) {
          record = true;
        }
      }
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

bool comp(Note a, Note b) { return a.time < b.time; };
class ShapeData {
public:
  int note;

  vector<pair<vec3, bool>> mesh;

  vec3 center;
  vec3 offColor;
  vec3 onColor;

  // Open GL stuff
  GLenum drawtype;
  GLfloat *vertices;
  GLfloat *colors;
  GLuint vertexbuffer;
  GLuint colorbuffer;

  InstrumentType inst;

  vector<Note> notes;

  ShapeData() {}
  ShapeData(int n, vector<pair<vec3, bool>> m, GLenum d, vec3 cen, vec3 Offcol,
            vec3 Oncol, vector<Note> no, InstrumentType i) {

    note = n;
    mesh = m;
    drawtype = d;
    center = cen;
    onColor = Oncol;
    offColor = Offcol;
    notes = no;
    inst = i;

    sort(notes.begin(), notes.end(), comp);
    if (inst != BACKGROUND)
      notes = subVec(notes, 1, notes.size());
  }

  void setup() {

    float borderProportion = 0.99;

    vertices = new GLfloat[mesh.size() * DIM];
    colors = new GLfloat[mesh.size() * DIM];

    for (int i = 0; i < mesh.size() * DIM; i += DIM) {
      vertices[i] = mapp(mesh[i / DIM].first.x, -1, 1, -borderProportion,
                         borderProportion);

      vertices[i] = mapp(vertices[i], -1, 1, -1, 1);
      vertices[i + 1] = mapp(mesh[i / DIM].first.y, -1, 1, -borderProportion,
                             borderProportion);
      vertices[i + 1] = mapp(vertices[i + 1], -1, 1, -1, 1);
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

  // t is a double from 0 to 1. It will weight the on color with the off color
  // based off of t. (1 is full onColor, 0 is full offColor, 1/2 is the average
  // color)
  void on(double t) {
    assert(t >= 0);
    assert(t <= 1);
    for (int i = 0; i < mesh.size() * DIM; i += DIM) {
      if (mesh[i / DIM].second) {
        colors[i] = onColor.x * t + offColor.x * (1 - t);
        colors[i + 1] = onColor.y * t + offColor.y * (1 - t);
        colors[i + 2] = onColor.z * t + offColor.z * (1 - t);
      } else {
        colors[i] = 0;
        colors[i + 1] = 0;
        colors[i + 2] = 0;
      }
    }
  }

  void update(double time, double pedal) {
    int lowerIndex = 0;
    if ((notes.size() == 0 || time < notes[0].time)) {
      for (int i = 0; i < mesh.size() * DIM; i += DIM) {

        colors[i] = 0;
        colors[i + 1] = 0;
        colors[i + 2] = 0;
      }
      return;
    }

    for (int i = 1; i < notes.size(); i++) {
      if (notes[i].time > time) {
        break;
      }
      lowerIndex = i;
    }
    if (inst == BACKGROUND) {
      if (notes[lowerIndex].onOff) {
        double t;
        double lastNote = notes[lowerIndex].time;
        double nextNote = notes[lowerIndex + 1].time;
        double x = time - lastNote;
        double y = time - nextNote;
        if (abs(x) < 1) {
          t = (tanh(x) / tanh(1) + 1) / 2;
        } else if (abs(y) < 1) {
          t = (-tanh(y) / tanh(1) + 1) / 2;
        } else {
          t = 1;
        }
        if (t > 1)
          t = 1;
        if (t < 0)
          t = 0;

        on(t);
      } else {
        double t;
        double lastNote = notes[lowerIndex].time;
        double nextNote = notes[lowerIndex + 1].time;
        double x = time - lastNote;
        double y = time - nextNote;
        if (abs(x) < 1) {
          t = ((-tanh(x)) / tanh(1) + 1) / 2;
        }
        if (abs(y) < 1) {
          t = (-tanh((-y)) / tanh(1) + 1) / 2;
        } else {
          t = 0;
        }

        if (t > 1)
          t = 1;
        if (t < 0)
          t = 0;

        on(t);
      }
    } else if (inst == SOLO) {
      if (notes[lowerIndex].onOff) {
        double t = 1 - (0.05 * (time - notes[lowerIndex].time));
        // decaying Note
        // cout << "onNote: " << t << " " << time << " " <<
        // notes[lowerIndex].time
        //      << " " << note << endl;
        on(t);
      } else {
        if (pedal > 0.1) {
          double t = 1 - (0.05 * (time - notes[lowerIndex - 1].time));
        }
        double a = notes[lowerIndex].time;

        double b =
            1.0 /
            (6 * (notes[lowerIndex].time - notes[lowerIndex - 1].time) + 1);

        double t = -3.1 * (time - a) + b;
        if (t < 0) {
          t = 0;
        }
        // cout << "offNote: " << t << " " << time << " " <<
        // notes[lowerIndex].time
        //      << " " << note << endl;
        on(t);
      }
    } else {
      if (notes[lowerIndex].onOff) {
        double t = 1 / (3 * (time - notes[lowerIndex].time) + 1);
        // decaying Note
        // cout << "onNote: " << t << " " << time << " " <<
        // notes[lowerIndex].time
        //      << " " << note << endl;
        on(t);
      } else {
        if (pedal > 0.1) {
          double t = 1 / (3 * (time - notes[lowerIndex - 1].time) + 1);
        }
        double a = notes[lowerIndex].time;

        double b =
            1.0 /
            (6 * (notes[lowerIndex].time - notes[lowerIndex - 1].time) + 1);

        double t = -3.1 * (time - a) + b;
        if (t < 0) {
          t = 0;
        }
        // cout << "offNote: " << t << " " << time << " " <<
        // notes[lowerIndex].time
        //      << " " << note << endl;
        on(t);
      }
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

vector<pair<vec3, bool>> makeMesh(ShapeType shape, vec3 center, double scale) {

  vector<pair<vec3, bool>> mesh;
  if (shape == CIRCLE) {
    for (int i = 0; i < CIRCLE_DEFINITION; i++) {
      mesh.push_back(
          make_pair(vec3(cos(i * 2 * M_PI / CIRCLE_DEFINITION) * scale,
                         sin(i * 2 * M_PI / CIRCLE_DEFINITION) * scale, 0) +
                        center,
                    true));
    }
    return mesh;
  }
  if (shape == DOT) {
    for (int i = 0; i < CIRCLE_DEFINITION; i++) {
      mesh.push_back(make_pair(
          vec3(cos(i * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0,
               sin(i * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0, 0) +
              center,
          true));

      mesh.push_back(make_pair(
          vec3(cos((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0,
               sin((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0, 0) +
              center,
          true));

      mesh.push_back(make_pair(center, true));

      mesh.push_back(make_pair(
          vec3(cos(i * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0,
               sin(i * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0, 0) +
              center,
          true));
      mesh.push_back(
          make_pair(vec3(cos(i * 2 * M_PI / CIRCLE_DEFINITION) * scale,
                         sin(i * 2 * M_PI / CIRCLE_DEFINITION) * scale, 0) +
                        center,
                    false));
      mesh.push_back(make_pair(
          vec3(cos((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale,
               sin((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale, 0) +
              center,
          false));

      mesh.push_back(make_pair(
          vec3(cos(i * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0,
               sin(i * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0, 0) +
              center,
          true));

      mesh.push_back(make_pair(
          vec3(cos((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale,
               sin((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale, 0) +
              center,
          false));

      mesh.push_back(make_pair(
          vec3(cos((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0,
               sin((i + 1) * 2 * M_PI / CIRCLE_DEFINITION) * scale / 2.0, 0) +
              center,
          true));
    }
    return mesh;
  }
  return {make_pair(vec3(0, 0, 0), true)};
}

vector<pair<vec3, bool>> pianoMesh(ShapeType shape, int key, int minKey,
                                   int maxKey, vec3 center, double leftBorder,
                                   double rightBorder) {
  // vec3 center = {0, 0, 0};
  // double leftBorder = -0.7;
  // double rightBorder = 0.7;
  vector<int> whiteKeys = {21, 23, 24,  26,  28,  29,  31,  33, 35, 36, 38,
                           40, 41, 43,  45,  47,  48,  50,  52, 53, 55, 57,
                           59, 60, 62,  64,  65,  67,  69,  71, 72, 74, 76,
                           77, 79, 81,  83,  84,  86,  88,  89, 91, 93, 95,
                           96, 98, 100, 101, 103, 105, 107, 108};

  double scale = 0.9 * (rightBorder - leftBorder) /
                 (getXPos(whiteKeys, maxKey) - getXPos(whiteKeys, minKey) + 1);

  vec3 wbadj = {mapp(getXPos(whiteKeys, key), getXPos(whiteKeys, minKey),
                     getXPos(whiteKeys, maxKey), leftBorder, rightBorder),
                find(whiteKeys.begin(), whiteKeys.end(), key) == whiteKeys.end()
                    ? scale
                    : 0,
                0};

  vector<pair<vec3, bool>> mesh = makeMesh(shape, center + wbadj, scale / 2);

  return mesh;
}

vector<pair<vec3, bool>> drumMesh(ShapeType shape, int key, vec3 center,
                                  double leftBorder, double rightBorder) {
  double scale = (leftBorder - rightBorder) * 0.25;
  double space = 0.05 * scale;
  vector<pair<vec3, bool>> mesh;
  cout << key << " " << center.x << ", " << center.y << endl;
  if (key == 36) {
    mesh =
        makeMesh(shape, center - vec3(scale * 0.75, scale * (-0.75) + space, 0),
                 scale * 3.1 / 4);
  }
  if (key == 37) {
    mesh = makeMesh(
        shape, center - vec3(scale * (-0.5) + space, scale * (-0.5) + space, 0),
        scale / 2);
  }
  if (key == 38) {
    mesh =
        makeMesh(shape, center - vec3(scale * (-0.2) - space, scale * (-1), 0),
                 scale / 4);
  }
  if (key == 39) {
    mesh = makeMesh(shape,
                    center - vec3(scale * (-1.2) + 2 * space, scale * (-1), 0),
                    scale / 4);
  }
  if (key == 42) {
    mesh =
        makeMesh(shape, center - vec3(scale * (-1) + space, scale * (0.5), 0),
                 scale / 2);
  }
  if (key == 44) {
    mesh = makeMesh(shape,
                    center - vec3(scale * (-1.3) + space, scale * (-0.25), 0),
                    scale / 4);
  }
  if (key == 46) {
    mesh = makeMesh(
        shape, center - vec3(scale * (-0.7) + space, scale * (1) + space, 0),
        scale / 4);
  }
  if (key == 41) {
    mesh = makeMesh(shape, center - vec3(0, scale * (0.5), 0), scale / 2);
  }
  if (key == 43) {
    mesh = makeMesh(shape, center - vec3(scale * (1) + space, scale * (0.5), 0),
                    scale / 2);
  }
  if (key == 45) {
    mesh = makeMesh(shape, center - vec3(space + scale * 2, 0, 0), scale / 2);
  }
  if (key == 47) {
    mesh =
        makeMesh(shape, center - vec3(space + scale * 2, -1 * scale - space, 0),
                 scale / 2);
  }
  return mesh;
}

vector<pair<vec3, bool>> backgroundMesh(ShapeType shape, int key, vec3 center,
                                        double leftBorder, double rightBorder) {
  double scale = (leftBorder - rightBorder);

  return makeMesh(shape, center, scale / 2);
}

class Instrument {
public:
  vector<ShapeData> shapes;
  map<int, vector<Note>> notes;
  vector<Note> pedal;

  Instrument(Args a, InstrumentType t) {
    MidiFile file;

    if (t == PIANO) {
      file.read(a.pianoPath);
      int minKey = 108;
      int maxKey = 21;

      for (int i = 3; i < file.getEventCount(0); i++) {
        if (file.getEvent(0, i).getP0() != 128 &&
            file.getEvent(0, i).getP0() !=
                144) // If not a note pressed or released (pedal or pitch
                     // bender)
        {
          pedal.push_back(Note(file.getEvent(0, i).getP0() == 144,
                               file.getTimeInSeconds(0, i) + a.pianoOffset * 2,
                               file.getEvent(0, i).getP2()));
          continue;
        }
        notes[file.getEvent(0, i).getP1()].push_back(
            Note(file.getEvent(0, i).getP0() == 144,
                 file.getTimeInSeconds(0, i) + a.pianoOffset * 2,
                 file.getEvent(0, i).getP2()));
        if (file.getEvent(0, i).getP1() < minKey) {
          minKey = file.getEvent(0, i).getP1();
        }
        if (file.getEvent(0, i).getP1() > maxKey) {
          maxKey = file.getEvent(0, i).getP1();
        }
      }
      for (int i = minKey; i <= maxKey; i++) {
        notes[i].push_back(Note(true, 0, 0));
      }
      for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
           i++) {
        shapes.push_back(ShapeData((*i).first,
                                   pianoMesh(DOT, (*i).first, minKey, maxKey,
                                             vec3(0, 0.5, 0), -0.85, -0.05),
                                   GL_TRIANGLES, vec3(0, 0, 0), vec3(0, 0.0, 0),
                                   vec3(0, 1, 0), (*i).second, t));
      }
    }
    if (t == BASS) {
      file.read(a.bassPath);
      int minKey = 108;
      int maxKey = 21;

      for (int i = 3; i < file.getEventCount(0); i++) {
        if (file.getEvent(0, i).getP0() != 128 &&
            file.getEvent(0, i).getP0() !=
                144) // If not a note pressed or released (pedal or pitch
                     // bender)
        {
          pedal.push_back(Note(file.getEvent(0, i).getP0() == 144,
                               file.getTimeInSeconds(0, i) + a.bassOffset * 2,
                               file.getEvent(0, i).getP2()));
          continue;
        }
        notes[file.getEvent(0, i).getP1()].push_back(
            Note(file.getEvent(0, i).getP0() == 144,
                 file.getTimeInSeconds(0, i) + a.bassOffset * 2,
                 file.getEvent(0, i).getP2()));
        if (file.getEvent(0, i).getP1() < minKey) {
          minKey = file.getEvent(0, i).getP1();
        }
        if (file.getEvent(0, i).getP1() > maxKey) {
          maxKey = file.getEvent(0, i).getP1();
        }
      }
      for (int i = minKey; i <= maxKey; i++) {
        notes[i].push_back(Note(true, 0, 0));
      }
      for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
           i++) {
        shapes.push_back(ShapeData((*i).first,
                                   pianoMesh(DOT, (*i).first, minKey, maxKey,
                                             vec3(0, -0.5, 0), -0.85, -0.05),
                                   GL_LINE_LOOP, vec3(0, 0, 0), vec3(0, 0, 0.0),
                                   vec3(1, 0, 0), (*i).second, t));
      }
    }
    if (t == DRUMS) {
      file.read(a.drumsPath);
      int minKey = 36;
      int maxKey = 47;
      for (int i = 3; i < file.getEventCount(0); i++) {
        // if (file.getEvent(0, i).getP0() != 128 &&
        //     file.getEvent(0, i).getP0() !=
        //         144) // If not a note pressed or released (pedal or pitch
        // bender)
        // {
        //   pedal.push_back(Note(file.getEvent(0, i).getP0() == 144,
        //                        file.getTimeInSeconds(0, i) + a.drumsOffset
        //                        * 2, file.getEvent(0, i).getP2()));
        //   continue;
        // }
        notes[file.getEvent(0, i).getP1()].push_back(
            Note(file.getEvent(0, i).getP0() == 144,
                 file.getTimeInSeconds(0, i) + a.drumsOffset * 2,
                 file.getEvent(0, i).getP2()));
        cout << file.getTimeInSeconds(0, i) << " " << a.drumsOffset * 2 << endl;
      }
      for (int i = minKey; i <= maxKey; i++) {
        notes[i].push_back(Note(true, 0, 0));
      }
      for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
           i++) {
        vec3 colorOverlap = vec3(1, 1.0 / 3, 1.0 / 3);
        if ((*i).first == 37 || (*i).first == 42) {
          colorOverlap = vec3(1, 1.0 / 3, 1.0 / 4);
        }
        if ((*i).first == 38 || (*i).first == 44) {
          colorOverlap = vec3(1, 1.0 / 4, 1.0 / 3);
        }
        shapes.push_back(ShapeData(
            (*i).first,
            drumMesh(DOT, (*i).first, vec3(0.35, -0.5, 0), 0.05, 0.85),
            GL_LINE_LOOP, vec3(0, 0, 0), vec3(0, 0, 0), colorOverlap,
            (*i).second, t));

        for (int s = 0; s < (*i).second.size(); s++) {
          cout << (*i).second[s].onOff << " " << (*i).second[s].time << endl;
        }
        cout << endl;
      }
    }
    if (t == SOLO) {
      file.read(a.soloPath);
      int minKey = 108;
      int maxKey = 21;

      for (int i = 3; i < file.getEventCount(0); i++) {
        if (file.getEvent(0, i).getP0() != 128 &&
            file.getEvent(0, i).getP0() !=
                144) // If not a note pressed or released (pedal or pitch
                     // bender)
        {
          pedal.push_back(Note(file.getEvent(0, i).getP0() == 144,
                               file.getTimeInSeconds(0, i) + a.soloOffset * 2,
                               file.getEvent(0, i).getP2()));
          continue;
        }
        notes[file.getEvent(0, i).getP1()].push_back(
            Note(file.getEvent(0, i).getP0() == 144,
                 file.getTimeInSeconds(0, i) + a.soloOffset * 2,
                 file.getEvent(0, i).getP2()));
        if (file.getEvent(0, i).getP1() < minKey) {
          minKey = file.getEvent(0, i).getP1();
        }
        if (file.getEvent(0, i).getP1() > maxKey) {
          maxKey = file.getEvent(0, i).getP1();
        }
      }
      for (int i = minKey; i <= maxKey; i++) {
        notes[i].push_back(Note(true, 0, 0));
      }
      for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
           i++) {
        shapes.push_back(ShapeData((*i).first,
                                   pianoMesh(DOT, (*i).first, minKey, maxKey,
                                             vec3(0, 0.5, 0), 0.05, 0.85),
                                   GL_LINE_LOOP, vec3(0, 0, 0),
                                   vec3(0.0, 0, 0.0), vec3(1, 1.0 / 3, 0),
                                   (*i).second, t));
      }
    }
    if (t == BACKGROUND) {
      file.read(a.backgroundPath);
      int minKey = 108;
      int maxKey = 21;

      for (int i = 3; i < file.getEventCount(0); i++) {
        notes[file.getEvent(0, i).getP1()].push_back(
            Note(file.getEvent(0, i).getP0() == 144,
                 file.getTimeInSeconds(0, i) + a.backgroundOffset * 2,
                 file.getEvent(0, i).getP2()));
        if (file.getEvent(0, i).getP1() < minKey) {
          minKey = file.getEvent(0, i).getP1();
        }
        if (file.getEvent(0, i).getP1() > maxKey) {
          maxKey = file.getEvent(0, i).getP1();
        }
      }
      for (int i = minKey; i <= maxKey; i++) {
        notes[i].push_back(Note(false, 0, 0));
      }
      for (map<int, vector<Note>>::iterator i = notes.begin(); i != notes.end();
           i++) {
        vec3 color(0.9, 0.2, 0.3);
        if ((*i).first != 50) {
          continue;
        }
        // if ((*i).first == 38) {
        //   color = vec3(0.02, 0.4, 0.8);
        // }
        // if ((*i).first == 39) {
        //   color = vec3(0.82, 0.4, 0.1);
        // }
        // if ((*i).first == 43) {
        //   color = vec3(0.22, 0.5, 0.7);
        // }
        // if ((*i).first == 45) {
        //   color = vec3(0.62, 0.7, 0.4);
        // }
        shapes.push_back(ShapeData(
            (*i).first, backgroundMesh(DOT, (*i).first, vec3(0, 0, 0), -1, 1),
            GL_LINE_LOOP, vec3(0, 0, 0), vec3(0.02, 0.4, 0.8), color,
            (*i).second, t));
      }
    }
  }

  void update(double cTime) {
    int lowerIndex = 0;
    double pedalDown = 0;
    if (!pedal.size() == 0) {
      for (int i = 0; i < pedal.size(); i++) {
        if (pedal[i].time > cTime) {
          break;
        }
        lowerIndex = i;
      }
      pedalDown = pedal[lowerIndex].intensity / 128.0;
    }

    for (int i = 0; i < shapes.size(); i++) {
      shapes[i].update(cTime, pedalDown);
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

  // void on(int note) {
  //   ShapeData *temp;
  //   for (int i = 0; i < shapes.size(); i++) {
  //     if (note == shapes[i].note) {
  //       temp = &shapes[i];
  //       break;
  //     }
  //   }
  //   temp->on();
  // }
  // void off(int note) {
  //   ShapeData *temp;
  //   for (int i = 0; i < shapes.size(); i++) {
  //     if (note == shapes[i].note) {
  //       temp = &shapes[i];
  //       break;
  //     }
  //   }
  //   temp->off();
  // }
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
    std::exit(1);
  }

  // We will ask it to specifically open an OpenGL 3.2 context
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Create a GLFW window
  int size = 1000;
  const char *windowName = "PrettyPictures";

  GLFWwindow *window = glfwCreateWindow(size, size, windowName, NULL, NULL);
  if (!window) {
    std::cerr << "ERROR: Failed to open GLFW window" << std::endl;
    glfwTerminate();
    std::exit(1);
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    std::cerr << "ERROR: Failed to initialize GLEW" << std::endl;
    glfwTerminate();
    std::exit(1);
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

  // Timer timer;
  // timer.start();
  double bpm = 102;
  double fps = 60;
  int totalFrames = 16000;
  int counter = 0;
  double currentTime = 0;
  // double prevTime = timer.elapsedSeconds();
  std::string command =
      "rm /home/oem/Documents/Code/Music-Visualization-CS4530/src/"
      "screenshots/*";
  std::system(command.c_str());

  while (!glfwWindowShouldClose(window) && counter < totalFrames) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 1st attribute buffer : vertices
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glUseProgram(programID);

    for (int i = 0; i < instruments.size(); i++) {
      instruments[i].update(currentTime);
      instruments[i].draw();
    }

    currentTime += bpm / (120.0 * fps);
    // prevTime = currentTime;
    if (a.record)
      screenshot(string(windowName),
                 "/home/oem/Documents/Code/Music-Visualization-CS4530/src/"
                 "screenshots/img-" +
                     to_string(counter) + ".png");
    // Swap buffers
    glfwSwapBuffers(window);

    // Screenshot

    counter++;
    cout << counter << endl;
    glfwPollEvents();
  }
  if (a.record) {
    // ffmpeg -framerate 5 -i img-%02d.png video.avi
    command = "ffmpeg -framerate " + to_string(fps) +
              " -i /home/oem/Documents/Code/Music-Visualization-CS4530/src/"
              "screenshots/img-%d.png  video.avi";
    std::system(command.c_str());
  }
  // Close OpenGL window and terminate GLFW
  glfwDestroyWindow(window);
  glfwTerminate();
  std::exit(EXIT_SUCCESS);
}

int main(int argc, char *args[]) {
  vector<Instrument> inst;
  Args a(args[1]);
  inst.push_back(Instrument(a, PIANO));
  inst.push_back(Instrument(a, BASS));
  inst.push_back(Instrument(a, DRUMS));
  inst.push_back(Instrument(a, SOLO));
  inst.push_back(Instrument(a, BACKGROUND));
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