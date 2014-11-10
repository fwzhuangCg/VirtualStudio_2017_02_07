/*
  Copyright ©2013 The Regents of the University of California
  (Regents). All Rights Reserved. Permission to use, copy, modify, and
  distribute this software and its documentation for educational,
  research, and not-for-profit purposes, without fee and without a
  signed licensing agreement, is hereby granted, provided that the
  above copyright notice, this paragraph and the following two
  paragraphs appear in all copies, modifications, and
  distributions. Contact The Office of Technology Licensing, UC
  Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley, CA 94720-1620,
  (510) 643-7201, for commercial licensing opportunities.

  IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT,
  INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
  LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
  DOCUMENTATION, EVEN IF REGENTS HAS BEEN ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

  REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
  DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS
  IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
  UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/

#include "io.h"
#include "util.h"
#include <cassert>
#include <cfloat>
#include <fstream>
#include <sstream>
#include <QTextStream>
#include <QFile>
#include <set>

using namespace std;


const int FILE_VERSION = 1;


// OBJ meshes

void get_valid_line (istream &in, string &line) {
    do
        getline(in, line);
    while (in && (line.length() == 0 || line[0] == '#'));
}

void triangle_to_obj (const string &inname, const string &outname) {
    fstream outfile(outname.c_str(), ios::out);
    { // nodes
        string filename = inname + ".node";
        fstream file(filename.c_str(), ios::in);
        string line;
        get_valid_line(file, line);
        stringstream linestream(line);
        int nv, dim, na, nb;
        linestream >> nv >> dim >> na >> nb;
        for (int i = 0; i < nv; i++) {
            get_valid_line(file, line);
            stringstream linestream(line);
            int index;
            linestream >> index;
            Vec2 u;
            linestream >> u[0] >> u[1];
            outfile << "v " << u[0] << " " << u[1] << " " << 0 << endl;
        }
    }
    { // eles
        string filename = inname + ".ele";
        fstream file(filename.c_str(), ios::in);
        string line;
        get_valid_line(file, line);
        stringstream linestream(line);
        int nt, nn, na;
        linestream >> nt >> nn >> na;
        for (int i = 0; i < nt; i++) {
            get_valid_line(file, line);
            stringstream linestream(line);
            int index;
            linestream >> index;
            int v0, v1, v2;
            linestream >> v0 >> v1 >> v2;
            outfile << "f " << v0+1 << " " << v1+1 << " " << v2+1 << endl;
        }
    }
}

vector<Face*> triangulate(const vector<Vert*> &verts);

void generate_obj(Mesh &mesh, const Vector2dVector &zdmesh)
{
	delete_mesh(mesh);
	float maxX = zdmesh[0].GetX(), maxY = zdmesh[0].GetY(), minX = maxX, minY = maxY;

	for(Vector2dVector::const_iterator iter = zdmesh.begin(); iter != zdmesh.end(); ++iter) {
		maxX = max(iter->GetX(), maxX);
		minX = min(iter->GetX(), minX);
		maxY = max(iter->GetY(), maxY);
		minY = min(iter->GetY(), minY);
	}

	for(Vector2dVector::const_iterator iter = zdmesh.begin(); iter != zdmesh.end();) {
		vector<Vert*> vertlist;
		int count = 3;
		while(count--) {
			float x = iter->GetX(), y = iter->GetY();
			Vert * v = new Vert(expand_xy(Vec2((x - minX) / (maxX - minX), (y - minY) / (maxY - minY))));
			Node * n = new Node(Vec3((x - minX - (maxX - minX) / 2) / 100, 0, (y - minY - (maxY - minY) / 2) / 100),
				Vec3((x - minX - (maxX - minX) / 2) / 100, 0, (y - minY - (maxY - minY) / 2) / 100),
				Vec3(0),
				0,
				0,
				false);
			connect(v, n);
			mesh.add(v);
			mesh.add(n);
			vertlist.push_back(v);
			++iter;
		}
		Face * f = new Face(vertlist[0], vertlist[1], vertlist[2], Mat3x3(1), Mat3x3(0), 0, 0);
		mesh.add(f);
	}
	mark_nodes_to_preserve(mesh);
	compute_ms_data(mesh);
}

struct WapNode {
	WapNode(Node * value) : node(value) {}
	Node * node;
};

bool operator<(const WapNode &t1, const WapNode &t2) {
	double a = 0.f;
	a += (abs(t1.node->x[0] - t2.node->x[0]) + abs(t1.node->x[1] - t2.node->x[1]) + abs(t1.node->x[2] - t2.node->x[2]));
	if(a <= 2e-4)
		return false;
	return ((t1.node->x[0] < t2.node->x[0]) || 
		((t1.node->x[0] == t2.node->x[0]) && (t1.node->x[1] < t2.node->x[1])) ||
		((t1.node->x[0] == t2.node->x[0]) && (t1.node->x[1] == t2.node->x[1]) && (t1.node->x[2] < t2.node->x[2])));
}

struct WapVert {
	WapVert(Vert * value) : vert(value) {}
	Vert * vert;
};

bool operator<(const WapVert &t1, const WapVert &t2) {
	double a = 0.f;
	a += (abs(t1.vert->u[0] - t2.vert->u[0]) + abs(t1.vert->u[1] - t2.vert->u[1]));
	if(a <= 2e-4)
		return false;
	return ((t1.vert->u[0] < t2.vert->u[0]) || 
		((t1.vert->u[0] == t2.vert->u[0]) && (t1.vert->u[1] < t2.vert->u[1])));
}

void load_obj (Mesh &mesh, const string &filename) {
    delete_mesh(mesh);
    QFile file(filename.c_str());
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
   		cout << "Error: failed to open file " << filename << endl;
		return;
    }
    set<WapNode> nodeset;
    set<WapVert> vertset;
    map<int, int> vindexmap;
    map<int, int> vtindexmap;
    int nv = 0, nvt = 0;
    while (!file.atEnd()) {
	QString line;
	line = file.readLine();
	QTextStream linestream(&line);
	QString keyword;
	linestream >> keyword;
	if (keyword == "vt") {
	    Vec2 u;
		linestream >> u[0] >> u[1];
			    // 利用set去重
		WapVert wv(new Vert(expand_xy(u)));
		set<WapVert>::iterator iter = vertset.find(wv);
		if(iter == vertset.end())
		{
			vtindexmap[nvt] = vertset.size();
				vertset.insert(wv);
				mesh.add(new Vert(expand_xy(u)));
			}
			else {
				vtindexmap[nvt] = iter->vert->index;
				//mesh.add(iter->vert, 1);
			}
			nvt++;
		} else if (keyword == "ms") {
			Vec3 u;
			linestream >> u[0] >> u[1] >> u[2];
			mesh.add(new Vert(u));
		} else if (keyword == "v") {
			Vec3 x;
			linestream >> x[0] >> x[1] >> x[2];
			// 利用set去重
			WapNode wn(new Node(x, x, Vec3(0), 0, 0, false));
			set<WapNode>::iterator iter = nodeset.find(wn);
			if(iter == nodeset.end())
			{
				vindexmap[nv] = nodeset.size();
				nodeset.insert(wn);
				mesh.add(new Node(x, x, Vec3(0), 0, 0, false));
			}
			else {
				vindexmap[nv] = iter->node->index;
				//mesh.add(iter->node, 1);
			}
			nv++;
		} else if (keyword == "ny") {
			Vec3 &y = mesh.nodes.back()->y;
			linestream >> y[0] >> y[1] >> y[2];
		} else if (keyword == "nv") {
			Vec3 &v = mesh.nodes.back()->v;
			linestream >> v[0] >> v[1] >> v[2];
		} else if (keyword == "nl") {
			linestream >> mesh.nodes.back()->label;
		} else if (keyword == "e") {
			int n0, n1;
			linestream >> n0 >> n1;
			mesh.add(new Edge(mesh.nodes[vindexmap[n0-1]], mesh.nodes[vindexmap[n1-1]], 0, 0));
		} else if (keyword == "ea") {
			linestream >> mesh.edges.back()->theta_ideal;
		} else if (keyword == "ed") {
			linestream >> mesh.edges.back()->damage;
		} else if (keyword == "ep") {
			linestream >> mesh.edges.back()->preserve;
		} else if (keyword == "f") {
			vector<Vert*> verts;
			vector<Node*> nodes;
			QString w;
			while (!((linestream >> w).atEnd())) {
				QTextStream wstream(&w);
				int v, n;
				char c;
				wstream >> n >> c >> v;
				nodes.push_back(mesh.nodes[vindexmap[n-1]]);
			   // if (!linestream.atEnd())                  // modified wunf 2013.12.11
					verts.push_back(mesh.verts[vtindexmap[v-1]]);
					/* else if (!nodes.back()->verts.empty())
					verts.push_back(nodes.back()->verts[0]);b
					else {
					verts.push_back(new Vert(project<2>(nodes.back()->x),
					nodes.back()->label));
					mesh.add(verts.back());
					}*/
			}
			for (int v = 0; v < verts.size(); v++)
			{
				if(!verts[v]->node)
					connect(verts[v], nodes[v]);
			}
			vector<Face*> faces = triangulate(verts);
			for (int f = 0; f < faces.size(); f++)
				mesh.add(faces[f]);
		} else if (keyword == "tm") {
			linestream >> mesh.faces.back()->flag;
		} else if (keyword == "tp") {
			Mat3x3 &S = mesh.faces.back()->Sp_bend;
            for (int i=0; i<3; i++) for (int j=0; j<3; j++)
            	linestream >> S(i,j);
		} else if (keyword == "ts") {
			Mat3x3 &S = mesh.faces.back()->Sp_str;
            for (int i=0; i<3; i++) for (int j=0; j<3; j++)
            	linestream >> S(i,j);
		} else if (keyword == "td") {
			linestream >> mesh.faces.back()->damage;
		}
	}
	file.close();
	mark_nodes_to_preserve(mesh);
	compute_ms_data(mesh);
}

void load_objs (vector<Mesh*> &meshes, const string &prefix) {
    for (int m = 0; m < (int)meshes.size(); m++)
        load_obj(*meshes[m], stringf("%s_%02d.obj", prefix.c_str(), m));
}

static double angle (const Vec3 &x0, const Vec3 &x1, const Vec3 &x2) {
    Vec3 e1 = normalize(x1 - x0);
    Vec3 e2 = normalize(x2 - x0);
    return acos(clamp(dot(e1, e2), -1., 1.));
}

vector<Face*> triangulate (const vector<Vert*> &verts) {
    int n = verts.size();
    double best_min_angle = 0;
    int best_root = -1;
    for (int i = 0; i < n; i++) {
        double min_angle = infinity;
        const Vert *vert0 = verts[i];
        for (int j = 2; j < n; j++) {
            const Vert *vert1 = verts[(i+j-1)%n], *vert2 = verts[(i+j)%n];
            min_angle=min(min_angle,
                          angle(vert0->node->x,vert1->node->x,vert2->node->x),
                          angle(vert1->node->x,vert2->node->x,vert0->node->x),
                          angle(vert2->node->x,vert0->node->x,vert1->node->x));
        }
        if (min_angle > best_min_angle) {
            best_min_angle = min_angle;
            best_root = i;
        }
    }
    int i = best_root;
    Vert* vert0 = verts[i];
    vector<Face*> tris;
    for (int j = 2; j < n; j++) {
        Vert *vert1 = verts[(i+j-1)%n], *vert2 = verts[(i+j)%n];
        tris.push_back(new Face(vert0, vert1, vert2, Mat3x3(1), Mat3x3(0), 0, 0));
    }
    return tris;
}

void save_obj (const Mesh &mesh, const string &filename) {
}

void save_objs (const vector<Mesh*> &meshes, const string &prefix) {
}

void save_transformation (const Transformation &tr, const string &filename) {
}

// Images

void flip_image (int w, int h, unsigned char *pixels);

void save_png (const char *filename, int width, int height,
               unsigned char *pixels, bool has_alpha = false);

#ifndef NO_OPENGL
void save_screenshot (const string &filename) {
}
#endif

void flip_image (int w, int h, unsigned char *pixels) {
    for (int j = 0; j < h/2; j++)
        for (int i = 0; i < w; i++)
            for (int c = 0; c < 3; c++)
                swap(pixels[(i+w*j)*3+c], pixels[(i+w*(h-1-j))*3+c]);
}

void save_png (const char *filename, int width, int height,
               unsigned char *pixels, bool has_alpha) {
}

void ensure_existing_directory (const std::string &path) {
}
