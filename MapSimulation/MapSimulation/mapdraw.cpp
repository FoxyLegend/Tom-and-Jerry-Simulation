//#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "toolfunctions.h"

using namespace std;

typedef struct {
	double x;
	double y;
	double vx;
	double vy;
	double d;
	double ss;
	bool dead;
} signal;

typedef struct {
	double x1;
	double y1;
	double x2;
	double y2;
} line;

typedef struct {
	double lat;
	double lon;
} node;

typedef struct {
	int *inodes, isize;
	double minx, miny, maxx, maxy;
} building, forest;

node *Nodes;
building *Buildings;
forest *Forests;

void signal_set(signal *s, double x, double y, double vx, double vy) {
	s->x = x;
	s->y = y;
	double len = sqrt(vx*vx + vy*vy);
	s->vx = vx / len;
	s->vy = vy / len;
	s->ss = 0;

	double nx, ny, d;
	nx = -vy;
	ny = vx;
	d = -(nx*x + ny*y);

	s->d = d;
}

/*
void kill_signal(signal *s) {
s->dead = true;
}
*/

#define kill_signal(s) (s->dead = true)
#define is_alive(s) (!(s->dead))

signal* init_signal() {
	signal *s = (signal*)malloc(sizeof(signal));
	s->x = 0;
	s->y = 0;
	s->vx = 0;
	s->vy = 0;
	s->ss = 0;
	s->dead = false;
	return s;
}

void signal_deepcpy(signal *a, signal *b) {
	if (a->dead) return;
	b->x = a->x;
	b->y = a->y;
	b->vx = a->vx;
	b->vy = a->vy;
	b->ss = a->ss;
	b->d = a->d;
}

line* init_line(double x1, double y1, double x2, double y2) {
	double nx, ny, len, d;
	line *l = (line*)malloc(sizeof(line));
	l->x1 = x1;
	l->y1 = y1;
	l->x2 = x2;
	l->y2 = y2;
	return l;
}

void signal_calc(signal *in, line *l, signal *out) {
	double Tnx = -in->vy;
	double Tny = in->vx;
	double Td = in->d;
	// p' = p1 + t(p2-p1), T(dot)p' = 0
	// t = -(T(dot)p1) / (T(dot)(p2 - p1))
	double tb = Tnx*(l->x2 - l->x1) + Tny*(l->y2 - l->y1);
	if (tb == 0.0) { // parallel
		out->ss = -1;
		return;
	}
	double t = -(Tnx*l->x1 + Tny*l->y1 + Td) / tb;
	double px, py, dx, dy, k;
	px = l->x1 + t*(l->x2 - l->x1);
	py = l->y1 + t*(l->y2 - l->y1);
	if (t > 1.0 || t < 0.0) {
		out->ss = -1;
		return;
	}
	//printf("ly1 = %f, ly2 = %f\n", l->y1, l->y2);
	//printf("px = %f, py = %f\n", px, py);
	//printf("t = %f\n", t);

	dx = px - in->x;
	if (dx != 0) {
		if (dx*in->vx < 0) {
			out->ss = -1;
			return;
		}
	}
	else {
		dy = py - in->y;
		if (dy != 0) {
			if (dy*in->vy < 0) {
				out->ss = -1;
				return;
			}
		}
		else {
			out->ss = -1;
			return;
		}
	}

	double lvx = l->x2 - l->x1;
	double lvy = l->y2 - l->y1;
	normalize(lvx, lvy);
	out->x = px;
	out->y = py;
	out->vx = in->vx - 2 * (in->vx*(-lvy) + in->vy*(lvx))*(-lvy);
	out->vy = in->vy - 2 * (in->vx*(-lvy) + in->vy*(lvx))*(lvx);
	out->ss = in->ss + dist(in->x, in->y, out->x, out->y);
	out->d = -((-out->vy)*px + (out->vx)*py);
}

void prt(signal *s) {
	printf("prt: p(%4.3f, %4.3f), v(%f, %f), s(%f)\n", s->x, s->y, s->vx, s->vy, s->ss);
}

void test1() {
	signal *in = init_signal();
	signal *out = init_signal();
	line *l = init_line(4, 4, 2, 6);

	signal_calc(in, l, out);
	prt(out);

	free(in);
	free(out);
	free(l);

	//	signal: p(3.000000, 5.000000), v(-0.894427, -0.447214), s(4.472136)
}

#define N 3600
#define EN 8
#define THRESHOLD 10
#define RADIUS 0.05
double PI = acos(-1.0);
line *edge[EN];
signal *sig[N], *out, *tout;
int selection_mode = 1; //generator: 0, detector: 1
double gx = 3, gy = 1; //generation point
double ax = 1.5, ay = .5; //accepting point

int nnum, bnum, fnum;
int e[EN][4]{
	{ 0., 0., 4., 0. },
	{ 4., 0., 4., 4. },
	{ 4., 4., 0., 4. },
	{ 0., 4., 0., 0. },
	{ 2., 0., 2., 2. },
	{ 0., 3., 3., 3. },
	{ 1., 2., 2., 2. },
	{ 1., 1., 1., 2. }
}; //if you edited, you should change EN

double d2r(double deg) {
	return deg * PI / 180.0;
}

void signal_calculation() {
	int i;
	for (i = 0; i < N; i++) {
		double r = d2r(360.0 * i / (double)N);
		signal_set(sig[i], gx, gy, cos(r), sin(r));
	}

	for (i = 0; i < N; i++) {
		signal *si = sig[i];
		int autoend = 20; //unknown error--> need to be fixed
		while (autoend-->0) {
			// calculate reflection
			int error = 1;
			tout->ss = -1;
			for (int i = 0; i < EN; i++) {
				signal_calc(si, edge[i], out);
				if (out->ss > 0) {
					error = 0;
					if (tout->ss < 0 || out->ss < tout->ss) {
						signal_deepcpy(out, tout);
					}
				}
			}

			if (error) {
				si->ss = -1;
				break;
			}

			// case of detection
			double tdist = dist(si->x, si->y, ax, ay);
			double t = (-si->vy)*ax + (si->vx)*ay + si->d;
			if (t < 0) t = -t; //absolute

							   // detection vs. reflection by distance
			if (t < RADIUS && tdist < dist(si->x, si->y, tout->x, tout->y)) {
				if (vec_equal(si->vx, si->vy, ax - si->x, ay - si->y)) {
					si->ss += tdist;
					break;
				}
				if (si->ss > THRESHOLD) {
					break;
				}
			}

			signal_deepcpy(tout, si);
		}
		if (autoend == 0) {
			printf("deg = %d : ", i);
			prt(si);
		}
	}
}

void load_file() {
	int i, count;
	FILE * fp;
	char stmp[255];
	char *pstr;
	char *context = NULL;
	char *token;
	const char del[3] = "\t\n";
	int nidx, bidx, fidx;

	fopen_s(&fp, "MapData.txt", "rt");
	if (fp != NULL)
	{
		nidx = bidx = fidx = 0;
		fscanf_s(fp, "i\t%d\t%d\t%d\n", &nnum, &bnum, &fnum);
		printf("%d, %d, %d\n", nnum, bnum, fnum);
		Nodes = (node*)malloc(sizeof(node)*nnum);
		Buildings = (building*)malloc(sizeof(building)*bnum);
		Forests = (forest*)malloc(sizeof(forest)*fnum);
		int tokidx;

		bool firstline = true;
		while (!feof(fp))
		{
			pstr = fgets(stmp, sizeof(stmp), fp);
			if (pstr == NULL) continue;
			if (pstr[0] == 'n') {
				sscanf_s(pstr, "n\t%lf\t%lf", &Nodes[nidx].lat, &Nodes[nidx].lon);
				nidx++;
			}
			if (pstr[0] == 'b') {
				count = 1;
				for (char *c = pstr; *c != NULL; c++) {
					if (*c == '\t') count++;
				}
				
				token = strtok_s(pstr, del, &context);
				while (token != NULL)
				{
					count++;
					token = strtok_s(NULL, del, &context);
				}

				Buildings[bidx].inodes = (int*)malloc(sizeof(int)*count);
				Buildings[bidx].isize = count;

				while (token != NULL)
				{
					int ti;
					sscanf_s(token, "%d", &ti);
					Buildings[bidx].inodes[tokidx] = ti;

					token = strtok_s(NULL, del, &context);
					tokidx++;
				}

				bidx++;
			}
			if (pstr[0] == 'f') {
				count = 1;
				for (char *c = pstr; *c != NULL; c++) {
					if (*c == '\t') count++;
				}

				token = strtok_s(pstr, del, &context);

				Forests[fidx].inodes = (int*)malloc(sizeof(int)*count);
				Forests[fidx].isize = count;

				while (token != NULL)
				{
					int ti;
					sscanf_s(token, "%d", &ti);
					Forests[fidx].inodes[tokidx] = ti;

					token = strtok_s(NULL, del, &context);
					tokidx++;
				}

				fidx++;
			}
		}
		fclose(fp);
	}
	else
	{
		//file not exist
	}
}

void initialize() {
	load_file();
}

void clean_up() {
	int i;
	free(Nodes);
	for (i = 0; i < bnum; i++) {
		if (Buildings[i].inodes != NULL)
			free(Buildings[i].inodes);
	}
	free(Buildings);
	for (i = 0; i < fnum; i++) {
		if (Forests[i].inodes != NULL)
			free(Forests[i].inodes);
	}
	free(Forests);
}

void display()
{
	int i, j, nidx;

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluPerspective(45, 1, 1, 1024);

	glColor3d(0, 0, 0);
	glBegin(GL_POLYGON);
	for (j = 0; j < bnum; j++) {
		for (i = 0; i < Buildings[j].isize; i++) {
			nidx = Buildings[j].inodes[i];
			glVertex3f(Nodes[nidx].lat, Nodes[nidx].lon, 0.0f);
		}
	}
	glColor3d(0, 0, 0);
	glBegin(GL_POLYGON);
	for (j = 0; j < fnum; j++) {
		for (i = 0; i < Forests[j].isize; i++) {
			nidx = Forests[j].inodes[i];
			glVertex3f(Nodes[nidx].lat, Nodes[nidx].lon, 0.0f);
		}
	}
	glEnd();

	glFlush();
}

/*********************************************************************************
* Call this part whenever user types keyboard.
* This part is called in main() function by registering on glutKeyboardFunc(onKeyPress).
**********************************************************************************/
void onKeyPress(unsigned char key, int x, int y) {
	printf("key %c pressed\n", key);
	if ((key == 'g')) { //generator
		selection_mode = 0;
	}
	if ((key == 'r')) { //receiver
		selection_mode = 1;
	}
	if ((key == 'd')) { //left
		if (selection_mode == 0) {
			gx += 0.1;
		}
		else {
			ax += 0.1;
		}
	}
	if ((key == 'a')) { //right
		if (selection_mode == 0) {
			gx -= 0.1;
		}
		else {
			ax -= 0.1;
		}
	}
	if ((key == 'w')) { //up
		if (selection_mode == 0) {
			gy += 0.1;
		}
		else {
			ay += 0.1;
		}
	}
	if ((key == 's')) { //down
		if (selection_mode == 0) {
			gy -= 0.1;
		}
		else {
			ay -= 0.1;
		}
	}

	glutPostRedisplay();
}


int main(int argc, char* argv[]) {
	
	initialize();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(512, 512);
	glutCreateWindow("CS408 SIGNAL TEST");
	glutDisplayFunc(display);
	glutKeyboardFunc(onKeyPress);
	glutMainLoop();
	clean_up();
	
	return 0;
}
