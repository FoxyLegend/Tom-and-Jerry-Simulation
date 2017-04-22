#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <array>

typedef struct {
	double x;
	double y;
} vector, point;

typedef struct signal {
	double x;
	double y;
	double vx;
	double vy;
	double d;
	double ss;
};

typedef struct line {
	double x1;
	double y1;
	double x2;
	double y2;
};

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

signal* init_signal() {
	signal *s = (signal*)malloc(sizeof(signal));
	s->x = 0;
	s->y = 0;
	s->vx = 0;
	s->vy = 0;
	s->ss = 0;
	return s;
}


void signal_deepcpy(signal *a, signal *b) {
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

double dist(double x1, double y1, double x2, double y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	return sqrt(dx*dx + dy*dy);
}

void normalize(double &vx, double &vy) {
	double len = sqrt(vx*vx + vy*vy);
	vx /= len;
	vy /= len;
}

int vec_equal(double vx, double vy, double ux, double uy) {
	if (vx != 0) {
		if (vx*ux > 0) {
			return 1;
		}
	}
	else {
		if (vy != 0) {
			if (uy*vy < 0) {
				return 1;
			}
		}
	}

	return 0;
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

#define N 360
#define EN 8
#define THRESHOLD 100
#define RADIUS 0.09
double PI = acos(-1.0);
line *edge[EN];
signal *sig[N], *out, *tout;
int selection_mode = 1; //generator: 0, detector: 1
double gx = 3, gy = 1; //generation point
double ax = 1.5, ay = .5; //accepting point

std::vector<std::array<double, 4>> e{
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
		int autoend = 10; //unknown error--> need to be fixed
		while (autoend-->0) {
			// calculate reflection
			tout->ss = THRESHOLD + 1;
			for (int i = 0; i < EN; i++) {
				signal_calc(si, edge[i], out);
				if (out->ss > 0) {
					if (out->ss < tout->ss) {
						signal_deepcpy(out, tout);
					}
				}
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
		if (autoend <= 0) {
			prt(si);
		}
	}
}

void initialize() {
	int i;
	for(i = 0; i < EN; i++)
		edge[i] = init_line(e[i][0], e[i][1], e[i][2], e[i][3]);

	out = init_signal();
	tout = init_signal();
	for (i = 0; i < N; i++) {
		double r = d2r(360.0 * i / (double)N);
		sig[i] = init_signal();
	}
}

void clean_up() {
	int i;
	for (i = 0; i < EN; i++) {
		free(edge[i]);
	}
	free(out);
	free(tout);
	for (i = 0; i < N; i++) {
		free(sig[i]);
	}
}

void draw_frame() {
	int i, j;
	glColor3d(1, 0, 0);
	for (i = 0; i < EN; i++) {
		glBegin(GL_LINES);
		glVertex3f(e[i][0], e[i][1], 0.0f);
		glVertex3f(e[i][2], e[i][3], 0.0f);
		glEnd();
	}
}

void display()
{
	signal_calculation();

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluPerspective(45, 1, 1, 1024);
	glTranslated(-2, -2, -5);

	draw_frame();
	glColor3d(0, 1, 0);
	glBegin(GL_QUADS);
	glVertex3f(gx + 0.1, gy - 0.1, 0.0f);
	glVertex3f(gx + 0.1, gy + 0.1, 0.0f);
	glVertex3f(gx - 0.1, gy + 0.1, 0.0f);
	glVertex3f(gx - 0.1, gy - 0.1, 0.0f);
	glEnd();


	glColor3d(0, 0, 1);
	for (int i = 0; i < N; i++) {
		signal *si = sig[i];
		if (si->ss < THRESHOLD) {
			glBegin(GL_LINES);
			glVertex3f(ax, ay, 0.0f);
			double m = 1/(si->ss*si->ss) * 5;
			double tx = m*(-si->vx);
			double ty = m*(-si->vy);
			glVertex3f(ax + tx, ay + ty, 0.0f);
			glEnd();
		}

	}

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