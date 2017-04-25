#ifndef TOOL_FUNC_H_
#define TOOL_FUNC_H_

double dist(double x1, double y1, double x2, double y2);
void normalize(double &vx, double &vy);
bool vec_equal(double vx, double vy, double ux, double uy);
bool vec_equal_dir(double vx, double vy, double ux, double uy);
bool compare(__int64 i, __int64 j);

#endif