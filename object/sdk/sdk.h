#pragma once

typedef bool BOOL;
typedef uint32_t DWORD;
#define FALSE false
#define TRUE true

#define PI M_PI
#define TWOPI M_PI * 2
#define HALFPI M_PI_2
#define Sin sin
#define Sqrt sqrt

#define ALLF 4
#define FOREVER INT_MAX
#define MAP_PLANAR 0
#define MAP_CYLINDRICAL 1
#define MAP_SPHERICAL 2
#define MAP_BALL 3
#define MAP_BOX 4
#define MAP_FACE 5

#define LimitValue(v, min, max) v = std::clamp(v, min, max)
#define SinCos(v, s, c) (*s) = sin(v); (*c) = cos(v);
#define TestAFlag(...) false
#define getAllVerts data
#define numFaces getNumFaces()
#define numVerts getNumVerts()
#define setEdgeVisFlags(...) begin()
#define setMatID(...) begin()
#define setSmGroup(...) begin()
#define setVerts(a, b, c) assign({(uint32_t)a, (uint32_t)b, (uint32_t)c})
#define setTVerts(a, b, c) assign({(uint32_t)a, (uint32_t)b, (uint32_t)c})

struct Matrix3
{
    float m[4][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    Matrix3(bool) {}
    void Scale(Point3 const& s)                     { m[0][0] = s.x; m[1][1] = s.y; m[2][2] = s.z; }
    void SetTrans(Point3 const& t)                  { m[3][0] = t.x; m[3][1] = t.y; m[3][2] = t.z; }
    void RotateZ(float r) {}
};

struct Mesh
{
    std::vector<Point3>& verts;
    std::vector<Face>& faces;
    std::vector<Point3>& tVerts;
    std::vector<Face>& tvFace;
    void setNumVerts(int nverts)                    { verts.resize(nverts); }
    int getNumVerts()                               { return int(verts.size()); }
    void setVert(int i, const Point3& p)            { verts[i] = p; }
    void setVert(int i, float x, float y, float z)  { verts[i] = { x, y, z }; }
    Point3& getVert(int i)                          { return verts[i]; }
    void setNumFaces(int nfaces)                    { faces.resize(nfaces); }
    int getNumFaces()                               { return int(faces.size()); }
    void setNumTVerts(int ntverts)                  { tVerts.resize(ntverts); }
    void setTVert(int i, float u, float v, float w) { tVerts[i] = { u, v, w }; }
    void setNumTVFaces(int nfaces)                  { tvFace.resize(nfaces); }
    void setFaceMtlIndex(int, int) {}
    void setSmoothFlags(bool) {}
    void ApplyUVWMap(int type, float utile, float vtile, float wtile, int uflip, int vflip, int wflip, int cap, const Matrix3 &tm, int channel = 1) {}
    void InvalidateGeomCache() {}
    void InvalidateStrips() {}
    void InvalidateTopologyCache() {}
};

struct IParamBlock
{
    void** data;
    void GetValue(int i, int, bool& value, int)     { memcpy(&value, data[i], sizeof(bool)); }
    void GetValue(int i, int, int& value, int)      { memcpy(&value, data[i], sizeof(int)); }
    void GetValue(int i, int, float& value, int)    { memcpy(&value, data[i], sizeof(float)); }
};

inline int MaxComponent(Point3 &p) {
    if (p.x > p.y && p.x > p.z)
        return 0;
    if (p.y > p.z)
        return 1;
    return 2;
}

inline void AddFace(Face *f,int a,int b,int c,int evis,int smooth_group)
{ f[0].setSmGroup(smooth_group);
  f[0].setMatID((MtlID)0);      /*default */
  if (evis==0) f[0].setEdgeVisFlags(1,1,0);
  else if (evis==1) f[0].setEdgeVisFlags(0,1,1);
  else if (evis==2) f[0].setEdgeVisFlags(0,0,1);
  else if (evis==ALLF) f[0].setEdgeVisFlags(1,1,1);
  else f[0].setEdgeVisFlags(1,0,1);
  f[0].setVerts(a,b,c);
}
