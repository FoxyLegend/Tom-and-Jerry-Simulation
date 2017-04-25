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
	double lat, lon, radius;
} building, forest;

node *Nodes;
building *Buildings;
forest *Forests;

void signal_set(signal *s, double x, double y, double vx, double vy, double ss, bool dead) {
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

	s->ss = ss;
	s->d = d;
	s->dead = dead;
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

void signal_calc(double lx1, double ly1, double lx2, double ly2, signal *in, signal *out) {
	double Tnx = -in->vy;
	double Tny = in->vx;
	double Td = in->d;
	// p' = p1 + t(p2-p1), T(dot)p' = 0
	// t = -(T(dot)p1) / (T(dot)(p2 - p1))
	double tb = Tnx*(lx2 - lx1) + Tny*(ly2 - ly1);
	if (tb == 0.0) { // parallel
		out->dead = true;
		return;
	}
	double t = -(Tnx*lx1 + Tny*ly1 + Td) / tb;
	double px, py, dx, dy, k;
	px = lx1 + t*(lx2 - lx1);
	py = ly1 + t*(ly2 - ly1);
	if (t > 1.0 || t < 0.0) {
		out->dead = true;
		return;
	}

	dx = px - in->x;
	if (dx != 0) {
		if (dx*in->vx < 0) {
			out->dead = true;
			return;
		}
	}
	else {
		dy = py - in->y;
		if (dy != 0) {
			if (dy*in->vy < 0) {
				out->dead = true;
				return;
			}
		}
		else {
			out->dead = true;
			return;
		}
	}

	double lvx = lx2 - lx1;
	double lvy = ly2 - ly1;
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

#define N 360
#define EN 8
#define THRESHOLD 100000
#define RADIUS 0.03
#define PNTSIZE 0.005
#define LINE_SIZE 10
int width = 800, height = 800;
double PI = acos(-1.0);
signal sig[N];
int selection_mode = 1; //generator: 0, detector: 1
double gx = -0.198, gy = 0.198; //generation point
double ax = -0.07, ay = -0.08; //accepting point

double scale = 130;
double mapx = 127.3623389;
double mapy = 36.370617;
double aspect = width / (double)height;

int nnum, bnum, fnum;

double d2r(double deg) {
	return deg * PI / 180.0;
}

void building_reflection(signal *sigin, signal *sigout) {
	double t, tb;
	double px, py;
	int n1, n2;
	signal sigtmp;
	
	//default
	sigout->dead = true;
	if (sigin->dead) return;

	int i, j, k;
	for (i = 0; i < bnum; i++) {
		// calculate reflection
		building *bd = &Buildings[i];

		t = (-sigin->vy)*bd->lon + (sigin->vx)*bd->lat + sigin->d;
		if (-bd->radius <= t && t <= bd->radius
			&& vec_equal_dir(sigin->vx, sigin->vy, (bd->lon - sigin->x), (bd->lat - sigin->y)))
		{
			for (k = 0; k < bd->isize - 1; k++)
			{
				n1 = bd->inodes[k];
				n2 = bd->inodes[k + 1];
				signal_calc(Nodes[n1].lon, Nodes[n1].lat, Nodes[n2].lon, Nodes[n2].lat, sigin, &sigtmp);
				if (!sigtmp.dead) {
					if (sigout->dead || sigout->ss < sigtmp.ss) {
						signal_deepcpy(&sigtmp, sigout); //tout = out
					}
				}
			}
		}
	}

	//if no reflection: sigout is dead
}

void forest_block(signal *sigin, signal *sigout) {
	double Tnx, Tny, Td;
	double d, dk, t, tb;
	double px, py, dx, dy;
	double lx1, ly1, lx2, ly2;
	int n1, n2;
	int i, j, k;
	sigout->dead = false;
	if (sigin->dead) {
		sigout->dead = true;
		return;
	}

	for (i = 0; i < fnum; i++) {
		// calculate reflection
		forest *fr = &Forests[i];

		d = (-sigin->vy)*fr->lon + (sigin->vx)*fr->lat + sigin->d;
		//possibly blocked if...
		dk = (fr->lon + sigin->vy*d - sigin->x) / sigin->vx;
		
		if (-fr->radius <= d && d <= fr->radius && dk > 0.0)
		{
			for (k = 0; k < fr->isize - 1; k++)
			{
				int n1 = fr->inodes[k];
				int n2 = fr->inodes[k + 1];

				lx1 = Nodes[n1].lon;
				ly1 = Nodes[n1].lat;
				lx2 = Nodes[n2].lon;
				ly2 = Nodes[n2].lat;

				Tnx = -sigin->vy;
				Tny = sigin->vx;
				Td = sigin->d;
				// p' = p1 + t(p2-p1), T(dot)p' = 0
				// t = -(T(dot)p1) / (T(dot)(p2 - p1))
				double tb = Tnx*(lx2 - lx1) + Tny*(ly2 - ly1);
				//if (tb == 0.0) { // parallel
					//return false;
				//}
				t = -(Tnx*lx1 + Tny*ly1 + Td) / tb;
				if (0.0 <= t && t <= 1.0) {
					px = lx1 + t*(lx2 - lx1);
					py = ly1 + t*(ly2 - ly1);

					double fdist = dist(sigin->x, sigin->y, px, py);
					if ((px - sigin->x) / sigin->vx > 0.0) {
						if (!sigout->dead || sigout->ss > fdist) {
							sigout->x = px;
							sigout->y = py;
							sigout->ss = fdist;
							sigout->dead = true;
						}
					}
				}
			}
		}
	}
}

void signal_calculation() {
	int i, j, k;
	double t, px, tk, tdist = 0, fdist = 0, bdist =0;
	signal sigtmp;
	bool possible, fblock = false;
	for (i = 0; i < N; i++) {
		double r = d2r(360.0 * i / (double)N);
		signal_set(&sig[i], gx, gy, cos(r), sin(r), 0, false);
	}
	for (i = 0; i < N; i++) {
		signal *si = &sig[i];
		int autoend = 20; //unknown error--> need to be fixed
		while (!si->dead && --autoend>0) {
			// case of detection
			possible = false;
			t = (-si->vy)*ax + (si->vx)*ay + si->d;
			if (-RADIUS <= t && t <= RADIUS) {
				px = ax + t*si->vy;
				tk = (px - si->x) / si->vx;
				if (tk > 0.0) {
					possible = true;
					tdist = dist(si->x, si->y, ax, ay);
				}
			}

			/*
			if (possible) {
				si->ss = tdist;
				break;
			}
			else {
				si->dead = true;
				break;
			}
			*/


			sigtmp.dead = false;
			forest_block(si, &sigtmp);
			fblock = sigtmp.dead;
			fdist = sigtmp.ss;
			if (fblock) {
				//fdist = dist(si->x, si->y, sigtmp.x, sigtmp.y);
				if (possible && tdist < fdist) {
					si->ss = tdist;
					break;
				}
				else {
					si->dead = true;
					break;
				}
			}
			else {
				if (possible) {
					si->ss = tdist;
					break;
				}
			}
			si->dead = true;
			break;

			
			/*
			if (possible && !fblock && tdist < fdist) {
				si->ss += tdist;
				break;
			}
			else {
				si->dead = true;
				break;
			}

			/*
			building_reflection(si, &sigtmp);
			if (sigtmp.dead) {
				if (fblock && fdist < tdist) {
					si->dead = true;
					break;
				}
			}
			else {
				bdist = dist(si->x, si->y, sigtmp.x, sigtmp.y);
			}
			if (possible && tdist < bdist) {
				si->ss += tdist;
				break;
			}

			signal_deepcpy(&sigtmp, si);
			*/
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

	bool firstline = true;
	bool isname = true;
	int ti;
	int tokidx;
	double mxx, mxy, mix, miy;

	fopen_s(&fp, "MapData.txt", "rt");
	if (fp != NULL)
	{
		nidx = bidx = fidx = 0;
		fscanf_s(fp, "i\t%d\t%d\t%d\n", &nnum, &bnum, &fnum);
		printf("%d, %d, %d\n", nnum, bnum, fnum);
		Nodes = (node*)malloc(sizeof(node)*nnum);
		Buildings = (building*)malloc(sizeof(building)*bnum);
		Forests = (forest*)malloc(sizeof(forest)*fnum);

		while (!feof(fp))
		{
			pstr = fgets(stmp, sizeof(stmp), fp);
			if (pstr == NULL) break;
			if (pstr[0] == 'n') {
				sscanf_s(pstr, "n\t%lf\t%lf", &Nodes[nidx].lat, &Nodes[nidx].lon);
				Nodes[nidx].lon = (Nodes[nidx].lon - mapx)*scale;
				Nodes[nidx].lat = (Nodes[nidx].lat - mapy)*scale*aspect;
				nidx++;
			}
			if (pstr[0] == 'b') {
				count = 0; //except name tag
				for (char *c = pstr; *c != NULL; c++) {
					if (*c == '\t') count++;
				}

				Buildings[bidx].inodes = (int*)malloc(sizeof(int)*count);
				Buildings[bidx].isize = count;
				mxx = mxy = -99999;
				mix = miy = 99999;

				token = strtok_s(pstr, del, &context);
				tokidx = 0;
				isname = true;
				while (token != NULL)
				{
					if (isname) {
						token = strtok_s(NULL, del, &context);
						isname = false;
						continue;
					}
					sscanf_s(token, "%d", &ti);
					Buildings[bidx].inodes[tokidx] = ti;
					if (mxx < Nodes[ti].lon)
						mxx = Nodes[ti].lon;
					if (mxy < Nodes[ti].lat)
						mxy = Nodes[ti].lat;
					if (mix > Nodes[ti].lon)
						mix = Nodes[ti].lon;
					if (miy > Nodes[ti].lat)
						miy = Nodes[ti].lat;

					token = strtok_s(NULL, del, &context);
					tokidx++;
				}
				Buildings[bidx].lon = (mxx + mix) / 2;
				Buildings[bidx].lat = (mxy + miy) / 2;
				Buildings[bidx].radius = sqrt((mxx - mix)*(mxx - mix) + (mxy - miy)*(mxy - miy)) / 2;

				bidx++;
			}
			if (pstr[0] == 'f') {
				count = 0;
				for (char *c = pstr; *c != NULL; c++) {
					if (*c == '\t') count++;
				}

				Forests[fidx].inodes = (int*)malloc(sizeof(int)*count);
				Forests[fidx].isize = count;
				mxx = mxy = -99999;
				mix = miy = 99999;

				token = strtok_s(pstr, del, &context);
				tokidx = 0;
				isname = true;
				while (token != NULL)
				{
					if (isname) {
						token = strtok_s(NULL, del, &context);
						isname = false;
						continue;
					}
					sscanf_s(token, "%d", &ti);
					Forests[fidx].inodes[tokidx] = ti;
					if (mxx < Nodes[ti].lon)
						mxx = Nodes[ti].lon;
					if (mxy < Nodes[ti].lat)
						mxy = Nodes[ti].lat;
					if (mix > Nodes[ti].lon)
						mix = Nodes[ti].lon;
					if (miy > Nodes[ti].lat)
						miy = Nodes[ti].lat;

					token = strtok_s(NULL, del, &context);
					tokidx++;
				}
				Forests[fidx].lon = (mxx + mix) / 2;
				Forests[fidx].lat = (mxy + miy) / 2;
				Forests[fidx].radius = sqrt((mxx - mix)*(mxx - mix) + (mxy - miy)*(mxy - miy)) / 2;

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
	signal_calculation();

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	
	glColor3d(0, 1, 0);
	for (j = 0; j < fnum; j++) {
		glBegin(GL_LINE_LOOP);
		for (i = 0; i < Forests[j].isize - 1; i++) {
			nidx = Forests[j].inodes[i];
			glVertex3f(Nodes[nidx].lon, Nodes[nidx].lat, 0.0f);
		}
		glEnd();
	}
	glColor3d(0.5, 0.5, 0.5);
	for (j = 0; j < bnum; j++) {
		glBegin(GL_LINE_LOOP);
		for (i = 0; i < Buildings[j].isize - 1; i++) {
			nidx = Buildings[j].inodes[i];
			glVertex3f(Nodes[nidx].lon, Nodes[nidx].lat, 0.0f);
		}
		glEnd();
	}

	glColor3d(1, 0, 0);
	glBegin(GL_POLYGON);
		glVertex3f(gx + PNTSIZE, gy - PNTSIZE, 0.0f);
		glVertex3f(gx + PNTSIZE, gy + PNTSIZE, 0.0f);
		glVertex3f(gx - PNTSIZE, gy + PNTSIZE, 0.0f);
		glVertex3f(gx - PNTSIZE, gy - PNTSIZE, 0.0f);
	glEnd();

	glColor3d(0, 0, 1);
	for (int i = 0; i < N; i++) {
		signal *si = &sig[i];
		if (!si->dead && si->ss > 0.0 && si->ss < THRESHOLD) {
			glBegin(GL_LINES);
			glVertex3f(ax, ay, 0.0f);
			double m = 1 / si->ss * LINE_SIZE;
			double tx = m*(-si->vx);
			double ty = m*(-si->vy);
			glVertex3f(ax + tx, ay + ty, 0.0f);
			glEnd();
		}
	}

	glColor3d(0, 0, 0.5);
	glBegin(GL_POLYGON);
		glVertex3f(ax + PNTSIZE, ay - PNTSIZE, 0.0f);
		glVertex3f(ax + PNTSIZE, ay + PNTSIZE, 0.0f);
		glVertex3f(ax - PNTSIZE, ay + PNTSIZE, 0.0f);
		glVertex3f(ax - PNTSIZE, ay - PNTSIZE, 0.0f);
	glEnd();


	glFlush();
}

void onMouseButton(int button, int state, int x, int y) {
	y = height - y - 1;
	double dx = 2 * (x - width*0.5) / width;
	double dy = 2 * (y - height*0.5) / height;
	//printf("mouse click on (%d, %d), (%lf, %lf)\n", x, y, dx, dy);
	double lon = mapx + dx / scale;
	double lat = mapy + dy / (scale * aspect);
	//printf("equivalent to (%lf, %lf)\n", lat, lon);

	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			gx = dx;
			gy = dy;
		}
	}
	else if (button == GLUT_RIGHT_BUTTON) {
		if (state == GLUT_DOWN) {
			ax = dx;
			ay = dy;
		}
	}

	glutPostRedisplay();
}

void onMouseDrag(int x, int y) {
}

/*********************************************************************************
* Call this part whenever user types keyboard.
* This part is called in main() function by registering on glutKeyboardFunc(onKeyPress).
**********************************************************************************/
void onKeyPress(unsigned char key, int x, int y) {
	if ((key == 'g')) { //generator
		selection_mode = 0;
	}
	if ((key == 'r')) { //receiver
		selection_mode = 1;
	}
	if ((key == 'd')) { //left
		if (selection_mode == 0) {
			gx += PNTSIZE;
		}
		else {
			ax += PNTSIZE;
		}
	}
	if ((key == 'a')) { //right
		if (selection_mode == 0) {
			gx -= PNTSIZE;
		}
		else {
			ax -= PNTSIZE;
		}
	}
	if ((key == 'w')) { //up
		if (selection_mode == 0) {
			gy += PNTSIZE;
		}
		else {
			ay += PNTSIZE;
		}
	}
	if ((key == 's')) { //down
		if (selection_mode == 0) {
			gy -= PNTSIZE;
		}
		else {
			ay -= PNTSIZE;
		}
	}

	glutPostRedisplay();
}


int main(int argc, char* argv[])
{
	initialize();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutCreateWindow("CS408 SIGNAL TEST");
	glutDisplayFunc(display);
	glutMouseFunc(onMouseButton);					// Register onMouseButton function to call that when user moves mouse.
	glutMotionFunc(onMouseDrag);					// Register onMouseDrag function to call that when user drags mouse.

	glutKeyboardFunc(onKeyPress);
	glutMainLoop();
	clean_up();

	return 0;
}
