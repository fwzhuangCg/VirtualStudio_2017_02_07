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


using namespace std;

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
			Vert * v = new Vert(Vec2((x - minX) / (maxX - minX), (y - minY) / (maxY - minY)));
			Node * n = new Node(Vec3((x - minX - (maxX - minX) / 2) / 100, 0, (y - minY - (maxY - minY) / 2) / 100));
			connectvn(v, n);
			mesh.add(v);
			mesh.add(n);
			vertlist.push_back(v);
			++iter;
		}
		Face * f = new Face(vertlist[0], vertlist[1], vertlist[2]);
		mesh.add(f);
	}
	mark_nodes_to_preserve(mesh);
	compute_ms_data(mesh);
}

void load_obj (Mesh &mesh, const string &filename) {
	delete_mesh(mesh);
	QFile file(filename.c_str());
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		cout << "Error: failed to open file " << filename << endl;
		return;
	}
	while (!file.atEnd()) {
		QString line;
		line = file.readLine();
		QTextStream linestream(&line);
		QString keyword;
		linestream >> keyword;
		if (keyword == "vt") {
			Vec2 u;
			linestream >> u[0] >> u[1];
			mesh.add(new Vert(u));
		} else if (keyword == "vl") {
			linestream >> mesh.verts.back()->label;
		} else if (keyword == "v") {
			Vec3 x;
			linestream >> x[0] >> x[1] >> x[2];
			mesh.add(new Node(x, Vec3(0)));
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
			mesh.add(new Edge(mesh.nodes[n0-1], mesh.nodes[n1-1]));
		} else if (keyword == "ea") {
			linestream >> mesh.edges.back()->theta_ideal;
		} else if (keyword == "ed") {
			linestream >> mesh.edges.back()->damage;
		} else if (keyword == "el") {
			linestream >> mesh.edges.back()->label;
		} else if (keyword == "f") {
			vector<Vert*> verts;
			vector<Node*> nodes;
			QString w;
			while (!((linestream >> w).atEnd())) {
				QTextStream wstream(&w);
				int v, n;
				char c;
				wstream >> n >> c >> v;
				nodes.push_back(mesh.nodes[n-1]);
			   // if (!linestream.atEnd())                  // modified wunf 2013.12.11
					verts.push_back(mesh.verts[v-1]);
					/* else if (!nodes.back()->verts.empty())
					verts.push_back(nodes.back()->verts[0]);
					else {
					verts.push_back(new Vert(project<2>(nodes.back()->x),
					nodes.back()->label));
					mesh.add(verts.back());
					}*/
			}
			for (int v = 0; v < verts.size(); v++)
				connectvn(verts[v], nodes[v]);
			vector<Face*> faces = triangulate(verts);
			for (int f = 0; f < faces.size(); f++)
				mesh.add(faces[f]);
		} else if (keyword == "tl" || keyword == "fl") {
			linestream >> mesh.faces.back()->label;
		} else if (keyword == "ts" || keyword == "fs") {
			Mat2x2 &S = mesh.faces.back()->S_plastic;
			linestream >> S(0,0) >> S(0,1) >> S(1,0) >> S(1,1);
		} else if (keyword == "td" || keyword == "fd") {
			linestream >> mesh.faces.back()->damage;
		}
	}
	file.close();
	mark_nodes_to_preserve(mesh);
	compute_ms_data(mesh);
}

void load_objs (vector<Mesh*> &meshes, const string &prefix) {
	for (int m = 0; m < meshes.size(); m++)
		load_obj(*meshes[m], stringff("%s_%02d.obj", prefix.c_str(), m));
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
		tris.push_back(new Face(vert0, vert1, vert2));
	}
	return tris;
}

void save_obj (const Mesh &mesh, const string &filename) {
	fstream file(filename.c_str(), ios::out);
	for (int v = 0; v < mesh.verts.size(); v++) {
		const Vert *vert = mesh.verts[v];
		file << "vt " << vert->u[0] << " " << vert->u[1] << endl;
		if (vert->label)
			file << "vl " << vert->label << endl;
	}
	for (int n = 0; n < mesh.nodes.size(); n++) {
		const Node *node = mesh.nodes[n];
		file << "v " << node->x[0] << " " << node->x[1] << " "
			 << node->x[2] << endl;
		if (norm2(node->x - node->y))
			file << "ny " << node->y[0] << " " << node->y[1] << " "
				 << node->y[2] << endl;
		if (norm2(node->v))
			file << "nv " << node->v[0] << " " << node->v[1] << " "
				 << node->v[2] << endl;
		if (node->label)
			file << "nl " << node->label << endl;
	}
	for (int e = 0; e < mesh.edges.size(); e++) {
		const Edge *edge = mesh.edges[e];
		if (edge->theta_ideal || edge->label) {
			file << "e " << edge->n[0]->index+1 << " " << edge->n[1]->index+1
				 << endl;
			if (edge->theta_ideal)
				file << "ea " << edge->theta_ideal << endl;
			if (edge->damage)
				file << "ed " << edge->damage << endl;
			if (edge->label)
				file << "el " << edge->label << endl;
		}
	}
	for (int f = 0; f < mesh.faces.size(); f++) {
		const Face *face = mesh.faces[f];
		file << "f " << face->v[0]->node->index+1 << "/" << face->v[0]->index+1
			 << " " << face->v[1]->node->index+1 << "/" << face->v[1]->index+1
			 << " " << face->v[2]->node->index+1 << "/" << face->v[2]->index+1
			 << endl;
		if (face->label)
			file << "tl " << face->label << endl;
		if (norm2_F(face->S_plastic)) {
			const Mat2x2 &S = face->S_plastic;
			file << "ts " << S(0,0) << " " << S(0,1) << " " << S(1,0) << " "
				 << S(1,1) << endl;
		}
		if (face->damage)
			file << "td " << face->damage << endl;
	}
}

void save_objs (const vector<Mesh*> &meshes, const string &prefix) {
	for (int m = 0; m < meshes.size(); m++)
		save_obj(*meshes[m], stringff("%s_%02d.obj", prefix.c_str(), m));
}

void save_transformation (const Transformation &tr, const string &filename) {
	FILE* file = fopen(filename.c_str(), "w");
	pair<Vec3, double> axis_angle = tr.rotation.to_axisangle();
	Vec3 axis = axis_angle.first;
	double angle = axis_angle.second * 180 / M_PI;
	fprintf(file, "<rotate angle=\"%f\" x=\"%f\" y=\"%f\" z=\"%f\"/>\n",
			angle, axis[0], axis[1], axis[2]);
	fprintf(file, "<scale value=\"%f\"/>\n", tr.scale);
	fprintf(file, "<translate x=\"%f\" y=\"%f\" z=\"%f\"/>\n",
			tr.translation[0], tr.translation[1], tr.translation[2]);
	fclose(file);
}

// Images

void flip_image (int w, int h, unsigned char *pixels);


void flip_image (int w, int h, unsigned char *pixels) {
	for (int j = 0; j < h/2; j++)
		for (int i = 0; i < w; i++)
			for (int c = 0; c < 3; c++)
				swap(pixels[(i+w*j)*3+c], pixels[(i+w*(h-1-j))*3+c]);
}