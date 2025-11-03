/* Coherent noise function over 1, 2 or 3 dimensions                    LuminariMUD*/
/* (copyright Ken Perlin) */

#ifndef _PERLIN_H_
#define _PERLIN_H_

#define MAX_GENERATED_NOISE 24

#define B 0x100
#define BM 0xff
#define N 0x1000
#define NP 12 /* 2^N */
#define NM 0xfff

#define s_curve(t) (t * t * (3. - 2. * t))
#define lerp(t, a, b) (a + t * (b - a))
#define setup(i, b0, b1, r0, r1) \
        t = vec[i] + N;          \
        b0 = ((int)t) & BM;      \
        b1 = (b0 + 1) & BM;      \
        r0 = t - (int)t;         \
        r1 = r0 - 1.;
#define at2(rx, ry) (rx * q[0] + ry * q[1])
#define at3(rx, ry, rz) (rx * q[0] + ry * q[1] + rz * q[2])

void init_perlin(int idx, int seed);
double noise1(int idx, double);
double noise2(int idx, double vec[2]);
double noise3(int idx, double vec[3]);
void normalize3(double v[3]);
void normalize2(double v[2]);

double PerlinNoise1D(int idx, double, double, double, int);
double PerlinNoise2D(int idx, double, double, double, double, int);
double PerlinNoise3D(int idx, double, double, double, double, double, int);
double RidgedMultifractal2D(int idx, double x, double y, double H, double lacunarity,
                            double octaves, double offset, double gain);

#endif
