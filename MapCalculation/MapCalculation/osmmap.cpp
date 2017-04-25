#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <array>

typedef struct node {
	int id;
	double x, y;
};

typedef struct building {
	int *nodes;
	int size;
};

std::vector<node> v;


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