#ifndef TOOL_FUNC_H_
#define TOOL_FUNC_H_

double dist(double x1, double y1, double x2, double y2);
void normalize(double &vx, double &vy);
void normalize(long double &vx, long double &vy);
bool compare(__int64 i, __int64 j);
bool fzero(double d);
bool fzero(long double d);

#endif