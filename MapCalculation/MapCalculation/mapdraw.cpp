//#define _CRT_SECURE_NO_WARNINGS

#include <GL/glut.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "toolfunctions.h"

#include <time.h>

typedef struct {
	long double x;
	long double y;
	long double vx;
	long double vy;
	long double d;
	long double ss;
	bool dead;
} signal;

typedef struct {
	long double x1;
	long double y1;
	long double x2;
	long double y2;
} line;

typedef struct {
	long double x;
	long double y;
} node;

typedef struct {
	int *inodes, isize;
	long double x, y, radius;
} polygon;


#define N 3600
#define THRESHOLD 100000
#define RADIUS 0.03
#define PNTSIZE 0.005
#define LINE_SIZE 0.02
#define AUTO_END 20
#define kill(s) (s->dead = true)
int width = 800, height = 800;
long double PI = acos(-1.0);
signal sig[N];
int selection_mode = 0; //generator: 0, detector: 1
long double gx = -0.198, gy = 0.198; //generation point
long double ax = -0.07, ay = -0.08; //accepting point

long double scale = 130;
long double mapx = 127.3623389;
long double mapy = 36.370617;
long double aspect = width / (long double)height;

int nnum, bnum, fnum;

node *Nodes;
polygon *Buildings;
polygon *Forests;
line reflecting[50000];
int nreflect = 0;
node meeting[50000];
int nmeeting = 0;
int point_mode = 0;

void signal_set(signal *s, long double x, long double y, long double vx, long double vy, long double ss, bool dead) {
	s->x = x;
	s->y = y;
	long double len = sqrt(vx*vx + vy*vy);
	s->vx = vx / len;
	s->vy = vy / len;
	s->ss = ss;

	long double nx, ny, d;
	nx = -vy;
	ny = vx;
	d = -(nx*x + ny*y);
	
	s->d = d;
	s->dead = dead;
}

void signal_deepcpy(signal *a, signal *b) {
	line *tl = &reflecting[nreflect++];
	tl->x1 = a->x;
	tl->y1 = a->y;
	tl->x2 = b->x;
	tl->y2 = b->y;

	node *m = &meeting[nmeeting++];
	m->x = a->x;
	m->y = a->y;


	b->x = a->x;
	b->y = a->y;
	b->vx = a->vx;
	b->vy = a->vy;
	b->ss = a->ss;

	b->d = -(b->x*(-b->vy) + b->y*(b->vx));
	b->dead = a->dead;


}

line* init_line(long double x1, double y1, long double x2, long double y2) {
	long double nx, ny, len, d;
	line *l = (line*)malloc(sizeof(line));
	l->x1 = x1;
	l->y1 = y1;
	l->x2 = x2;
	l->y2 = y2;
	return l;
}

void signal_calc(long double lx1, long double ly1, long double lx2, long double ly2, signal *in, signal *out) {
	long double Tnx = -in->vy;
	long double Tny = in->vx;
	long double Td = in->d;
	// p' = p1 + t(p2-p1), T(dot)p' = 0
	// t = -(T(dot)p1) / (T(dot)(p2 - p1))
	long double tb = Tnx*(lx2 - lx1) + Tny*(ly2 - ly1);
	out->dead = true;
	if (fzero(tb)) return; // parallel

	long double t = -(Tnx*lx1 + Tny*ly1 + Td) / tb;
	long double px, py, dx, dy, k;
	px = lx1 + t*(lx2 - lx1);
	py = ly1 + t*(ly2 - ly1);
	if (0.0 <= t && t <= 1.0) {
		if (!fzero(in->vx)) {
			k = (px - in->x) / in->vx;
		}
		else {
			k = (py - in->y) / in->vy;
		}

		if (k > 0) {
			long double lvx = lx2 - lx1;
			long double lvy = ly2 - ly1;
			normalize(lvx, lvy);
			out->x = px;
			out->y = py;
			out->vx = in->vx - 2 * (in->vx*(-lvy) + in->vy*(lvx))*(-lvy);
			out->vy = in->vy - 2 * (in->vx*(-lvy) + in->vy*(lvx))*(lvx);
			out->ss = dist(in->x, in->y, out->x, out->y); //in->ss + 
			out->d = -((-out->vy)*out->x + (out->vx)*out->y);
			out->dead = false;
		}
	}
}

void prt(signal *s) {
	printf("prt: p(%4.3f, %4.3f), v(%f, %f), s(%f)\n", s->x, s->y, s->vx, s->vy, s->ss);
}

long double d2r(long double deg) {
	return deg * PI / 180.0;
}

void forest_block(signal *sigin, signal *sigout) {
	long double Tnx, Tny, Td;
	long double d, dk, t, tb;
	long double px, py, dx, dy;
	long double lx1, ly1, lx2, ly2;
	int n1, n2;
	int i, j, k;
	long double kdist, tk;

	if (sigin->dead) {
		sigout->dead = true;
		return;
	}
	sigout->dead = false;

	for (i = 0; i < fnum; i++) {
		// calculate reflection
		polygon *p = &Forests[i];

		d = (-sigin->vy)*p->x + (sigin->vx)*p->y + sigin->d;
		//possibly blocked if...
		if (-p->radius <= d && d <= p->radius)
		{
			for (k = 0; k < p->isize - 1; k++)
			{
				int n1 = p->inodes[k];
				int n2 = p->inodes[k + 1];

				lx1 = Nodes[n1].x;
				ly1 = Nodes[n1].y;
				lx2 = Nodes[n2].x;
				ly2 = Nodes[n2].y;

				Tnx = -sigin->vy;
				Tny = sigin->vx;
				Td = sigin->d;
				// p' = p1 + t(p2-p1), T(dot)p' = 0
				// t = -(T(dot)p1) / (T(dot)(p2 - p1))
				long double tb = Tnx*(lx2 - lx1) + Tny*(ly2 - ly1);
				if (fzero(tb)) { // parallel
					break;
				}
				t = -(Tnx*lx1 + Tny*ly1 + Td) / tb;
				if (0.0 < t && t < 1.0) {
					px = lx1 + t*(lx2 - lx1);
					py = ly1 + t*(ly2 - ly1);

					if (!fzero(sigin->vx)) {
						tk = (px - sigin->x) / sigin->vx;
					}
					else {
						tk = (py - sigin->y) / sigin->vy;
					}

					if (tk > 0) {
						kdist = dist(sigin->x, sigin->y, px, py);
						if (!sigout->dead || sigout->ss > kdist) { //if marked as alive
							sigout->x = px;
							sigout->y = py;
							sigout->ss = kdist;
							sigout->dead = true;
						}
					}
				}
			}
		}
	}
}

void building_reflection(signal *sigin, signal *sigout) {
	long double Tnx, Tny, Td;
	long double d, dk, t, tb;
	long double px, py, dx, dy;
	long double lx1, ly1, lx2, ly2;
	int n1, n2;
	int i, j, k;
	long double kdist, tk;

	if (sigin->dead) {
		sigout->dead = true;
		return;
	}
	sigout->dead = true;

	for (i = 0; i < bnum; i++) {
		// calculate reflection
		polygon *p = &Buildings[i];

		d = (-sigin->vy)*p->x + (sigin->vx)*p->y + sigin->d;
		//possibly blocked if...
		if (-p->radius <= d && d <= p->radius)
		{
			for (k = 0; k < p->isize - 1; k++)
			{
				int n1 = p->inodes[k];
				int n2 = p->inodes[k + 1];

				lx1 = Nodes[n1].x;
				ly1 = Nodes[n1].y;
				lx2 = Nodes[n2].x;
				ly2 = Nodes[n2].y;

				Tnx = -sigin->vy;
				Tny = sigin->vx;
				Td = sigin->d;
				// p' = p1 + t(p2-p1), T(dot)p' = 0
				// t = -(T(dot)p1) / (T(dot)(p2 - p1))

				long double tb = Tnx*(lx2 - lx1) + Tny*(ly2 - ly1);
				if (fzero(tb)) { // parallel
					break;
				}
				t = -(Tnx*lx1 + Tny*ly1 + Td) / tb;

				if (0.0 < t && t < 1.0) {
					px = lx1 + t*(lx2 - lx1);
					py = ly1 + t*(ly2 - ly1);
					

					if (!fzero(sigin->vx)) {
						tk = (px - sigin->x) / sigin->vx;
					}
					else if (!fzero(sigin->vy)) {
						tk = (py - sigin->y) / sigin->vy;
					}
					else {
						sigout->dead = true;
						break;
					}

					if (tk > 0) {
						long double lnx = -(ly2 - ly1);
						long double lny = lx2 - lx1;

						if (fzero(lnx*sigin->vx + lny*sigin->vy)) {
							kdist = tk;
						}
						else {
							kdist = dist(sigin->x, sigin->y, px, py);
						}

						if (sigout->dead || sigout->ss > kdist) { //if marked as alive
							normalize(lnx, lny);
							long double nv = sigin->vx*lnx + sigin->vy*lny;
							sigout->x = px;
							sigout->y = py;
							sigout->vx = sigin->vx - 2 * nv * lnx;
							sigout->vy = sigin->vy - 2 * nv * lny;
							sigout->ss = kdist;
							sigout->dead = false;

						}
					}
				}
			}
		}
	}
}

void signal_calculation() {
	nreflect = 0;
	nmeeting = 0;
	int i, j, k;
	long double t, px, py, tk, tdist = 0, kdist = 0;
	signal sigref, sigblk;
	bool possible, block = false, reflect = false;
	for (i = 0; i < N; i++) {
		long double r = d2r(360.0 * i / (long double)N);
		signal_set(&sig[i], gx, gy, cos(r), sin(r), 0, false);
	}
	for (i = 0; i < N; i++) {
		signal *si = &sig[i];
		int autoend = 0; //unknown error--> need to be fixed
		while (!si->dead && autoend < 4) {
			autoend++;
			si->d = -((-si->vy)*si->x + si->vx*si->y);
			/*
			if (fzero(sig[i].vx) || fzero(sig[i].vy)) { //currently there is bug
				sig[i].dead = true;
				break;
			}
			*/
			// case of detection
			possible = false;
			t = (-si->vy)*ax + (si->vx)*ay + si->d;
			if (-RADIUS <= t && t <= RADIUS) {
				if (!fzero(si->vx)) {
					px = ax + t*si->vy;
					tk = (px - si->x) / si->vx;
				}
				else {
					py = ay + t*(-si->vx);
					tk = (py - si->y) / si->vy;
				}

				if (tk > 0) {
					possible = true;
					tdist = dist(si->x, si->y, ax, ay);
				}
			}
			
			// reflection test
			building_reflection(si, &sigref);
			// blocking test
			forest_block(si, &sigblk);
			
			if (sigblk.dead) {
				if (!sigref.dead && sigref.ss < sigblk.ss) {
					signal_deepcpy(&sigref, si);
				}
				else {
					kill(si);
					break;
				}
			}
			else {
				signal_deepcpy(&sigref, si);
			}
			continue;

			/*
			if (!sigref.dead) {
				if (sigblk.dead) {
					if (possible && tdist < sigref.ss && tdist < sigblk.ss) {
						si->ss += tdist;
						break;
					}
					if (sigref.ss < sigblk.ss) {
						sigref.ss += si->ss;
						signal_deepcpy(&sigref, si);
						continue;
					}
					else {
						si->dead = true;
						break;
					}
				}
				else {
					if (possible && tdist < sigref.ss) {
						si->ss += tdist;
						break;
					}
					else {
						sigref.ss += si->ss;
						signal_deepcpy(&sigref, si);
						continue;
					}
				}
			}
			else {
				if (sigblk.dead) {
					if (possible && tdist < sigblk.ss) {
						si->ss += tdist;
						break;
					}
					else {
						si->dead = true;
						break;
					}
				}
			}

			*/

			if (possible) {
				si->ss += tdist;
				break;
			}
			else {
				si->dead = true;
				break;
			}
			
		}
		if (autoend == AUTO_END) {
			si->dead = true;
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
	long double mxx, mxy, mix, miy;

	fopen_s(&fp, "MapData.txt", "rt");
	if (fp != NULL)
	{
		nidx = bidx = fidx = 0;
		fscanf_s(fp, "i\t%d\t%d\t%d\n", &nnum, &bnum, &fnum);
		printf("%d, %d, %d\n", nnum, bnum, fnum);
		Nodes = (node*)malloc(sizeof(node)*nnum);
		Buildings = (polygon*)malloc(sizeof(polygon)*bnum);
		Forests = (polygon*)malloc(sizeof(polygon)*fnum);

		while (!feof(fp))
		{
			pstr = fgets(stmp, sizeof(stmp), fp);
			if (pstr == NULL) break;
			if (pstr[0] == 'n') {
				sscanf_s(pstr, "n\t%lf\t%lf", &Nodes[nidx].y, &Nodes[nidx].x);
				Nodes[nidx].x = (Nodes[nidx].x - mapx)*scale;
				Nodes[nidx].y = (Nodes[nidx].y - mapy)*scale*aspect;
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
					if (mxx < Nodes[ti].x)
						mxx = Nodes[ti].x;
					if (mxy < Nodes[ti].y)
						mxy = Nodes[ti].y;
					if (mix > Nodes[ti].x)
						mix = Nodes[ti].x;
					if (miy > Nodes[ti].y)
						miy = Nodes[ti].y;

					token = strtok_s(NULL, del, &context);
					tokidx++;
				}
				Buildings[bidx].x = (mxx + mix) / 2;
				Buildings[bidx].y = (mxy + miy) / 2;
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
					if (mxx < Nodes[ti].x)
						mxx = Nodes[ti].x;
					if (mxy < Nodes[ti].y)
						mxy = Nodes[ti].y;
					if (mix > Nodes[ti].x)
						mix = Nodes[ti].x;
					if (miy > Nodes[ti].y)
						miy = Nodes[ti].y;

					token = strtok_s(NULL, del, &context);
					tokidx++;
				}
				Forests[fidx].x = (mxx + mix) / 2;
				Forests[fidx].y = (mxy + miy) / 2;
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

	clock_t tic = clock();

	int i, j, in1, in2;
	signal_calculation();

	clock_t toc = clock();

	printf("Calculation Elapsed: %d ms.\n", (toc - tic));
	tic = toc;

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	//

	glColor3d(0.9, 0.7, 0.9);
	for (i = 0; i < nreflect; i++) {
		line *tl = &reflecting[i];
		glBegin(GL_LINES);
		glVertex3f(tl->x1, tl->y1, 0.0f);
		glVertex3f(tl->x2, tl->y2, 0.0f);
		glEnd();
	}
	/*
	for (i = 0; i < N; i++) {
		signal *si = &sig[i];
		if (!si->dead && si->ss > 0 && si->ss < THRESHOLD) {
			glColor3d(0.7, 0.7, 0.9);
			glBegin(GL_LINES);
			glVertex3f(si->x, si->y, 0.0f);
			glVertex3f(ax, ay, 0.0f);
			glEnd();
			glColor3d(0.7, 0.9, 0.9);
			glBegin(GL_LINES);
			glVertex3f(ax, ay, 0.0f);
			glVertex3f(ax - 0.1*si->vx, ay - 0.1*si->vy, 0.0f);
			glEnd();
		}

	}*/

	for (i = 0; i < nmeeting; i++) {
		node *n = &meeting[i];
		long double kx = n->x;
		long double ky = n->y;
		glColor3d(0, 0.5, 0.5);
		glBegin(GL_POLYGON);
		glVertex3f(kx + PNTSIZE, ky - PNTSIZE, 0.0f);
		glVertex3f(kx + PNTSIZE, ky + PNTSIZE, 0.0f);
		glVertex3f(kx - PNTSIZE, ky + PNTSIZE, 0.0f);
		glVertex3f(kx - PNTSIZE, ky - PNTSIZE, 0.0f);
		glEnd();
	}

	//


	glColor3d(0, 1, 0);
	for (j = 0; j < fnum; j++) {
		for (i = 0; i < Forests[j].isize - 1; i++) {
			in1 = Forests[j].inodes[i];
			in2 = Forests[j].inodes[i+1];
			glBegin(GL_LINES);
			glVertex3f(Nodes[in1].x, Nodes[in1].y, 0.0f);
			glVertex3f(Nodes[in2].x, Nodes[in2].y, 0.0f);
			glEnd();
			if (point_mode) {
				glBegin(GL_POLYGON);
				glVertex3f(Nodes[in1].x + PNTSIZE, Nodes[in1].y - PNTSIZE, 0.0f);
				glVertex3f(Nodes[in1].x + PNTSIZE, Nodes[in1].y + PNTSIZE, 0.0f);
				glVertex3f(Nodes[in1].x - PNTSIZE, Nodes[in1].y + PNTSIZE, 0.0f);
				glVertex3f(Nodes[in1].x - PNTSIZE, Nodes[in1].y - PNTSIZE, 0.0f);
				glEnd();
			}
		}
	}

	glColor3d(0.5, 0.5, 0.5);
	for (j = 0; j < bnum; j++) {
		for (i = 0; i < Buildings[j].isize - 1; i++) {
			in1 = Buildings[j].inodes[i];
			in2 = Buildings[j].inodes[i + 1];
			glBegin(GL_LINES);
			glVertex3f(Nodes[in1].x, Nodes[in1].y, 0.0f);
			glVertex3f(Nodes[in2].x, Nodes[in2].y, 0.0f);
			glEnd();
			if (point_mode) {
				glBegin(GL_POLYGON);
				glVertex3f(Nodes[in1].x + PNTSIZE, Nodes[in1].y - PNTSIZE, 0.0f);
				glVertex3f(Nodes[in1].x + PNTSIZE, Nodes[in1].y + PNTSIZE, 0.0f);
				glVertex3f(Nodes[in1].x - PNTSIZE, Nodes[in1].y + PNTSIZE, 0.0f);
				glVertex3f(Nodes[in1].x - PNTSIZE, Nodes[in1].y - PNTSIZE, 0.0f);
				glEnd();
			}
		}
	}

	/*
	glColor3d(0, 0, 1);
	for (int i = 0; i < N; i++) {
		signal *si = &sig[i];
		if (!si->dead && si->ss > 0.0 && si->ss < THRESHOLD) {
			glBegin(GL_LINES);
			glVertex3f(ax, ay, 0.0f);
			long double m = 1 / si->ss * LINE_SIZE;
			long double tx = m*(-si->vx);
			long double ty = m*(-si->vy);
			glVertex3f(ax + tx, ay + ty, 0.0f);
			glEnd();
		}
	}
	*/


	glColor3d(0, 0, 0.5);
	glBegin(GL_POLYGON);
	glVertex3f(ax + PNTSIZE, ay - PNTSIZE, 0.0f);
	glVertex3f(ax + PNTSIZE, ay + PNTSIZE, 0.0f);
	glVertex3f(ax - PNTSIZE, ay + PNTSIZE, 0.0f);
	glVertex3f(ax - PNTSIZE, ay - PNTSIZE, 0.0f);
	glEnd();

	glColor3d(1, 0, 0);
	glBegin(GL_POLYGON);
	glVertex3f(gx + PNTSIZE, gy - PNTSIZE, 0.0f);
	glVertex3f(gx + PNTSIZE, gy + PNTSIZE, 0.0f);
	glVertex3f(gx - PNTSIZE, gy + PNTSIZE, 0.0f);
	glVertex3f(gx - PNTSIZE, gy - PNTSIZE, 0.0f);
	glEnd();

	glFlush();

	toc = clock();

	printf("Drawing Elapsed: %d ms.\n", (toc - tic));
}

void onMouseButton(int button, int state, int x, int y) {
	y = height - y - 1;
	long double dx = 2 * (x - width*0.5) / width;
	long double dy = 2 * (y - height*0.5) / height;
	//printf("mouse click on (%d, %d), (%lf, %lf)\n", x, y, dx, dy);
	long double lon = mapx + dx / scale;
	long double lat = mapy + dy / (scale * aspect);
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
	if ((key == 'p')) { //receiver
		point_mode = !point_mode;
	}
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
