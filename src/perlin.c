/* Coherent noise function over 1, 2 or 3 dimensions                 LuminariMUD */
/* (copyright Ken Perlin) */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "perlin.h"

static int p[MAX_GENERATED_NOISE][B + B + 2];
static double g3[MAX_GENERATED_NOISE][B + B + 2][3];
static double g2[MAX_GENERATED_NOISE][B + B + 2][2];
static double g1[MAX_GENERATED_NOISE][B + B + 2];
// static int start = 0;

double noise1(int idx, double arg)
{
   int bx0, bx1;
   double rx0, rx1, sx, t, u, v, vec[1];

   vec[0] = arg;
   /*   if (start) {
      start = 0;
      init_perlin(time(0));
   }
*/
   setup(0, bx0, bx1, rx0, rx1);

   sx = s_curve(rx0);
   u = rx0 * g1[idx][p[idx][bx0]];
   v = rx1 * g1[idx][p[idx][bx1]];

   return (lerp(sx, u, v));
}

double noise2(int idx, double vec[2])
{
   int bx0, bx1, by0, by1, b00, b10, b01, b11;
   double rx0, rx1, ry0, ry1, *q, sx, sy, a, b, t, u, v;
   int i, j;

   /*
   if (start) {
      start = 0;
      init_perlin(time(0));
   }
*/

   setup(0, bx0, bx1, rx0, rx1);
   setup(1, by0, by1, ry0, ry1);

   i = p[idx][bx0];
   j = p[idx][bx1];

   b00 = p[idx][i + by0];
   b10 = p[idx][j + by0];
   b01 = p[idx][i + by1];
   b11 = p[idx][j + by1];

   sx = s_curve(rx0);
   sy = s_curve(ry0);

   q = g2[idx][b00];
   u = at2(rx0, ry0);
   q = g2[idx][b10];
   v = at2(rx1, ry0);
   a = lerp(sx, u, v);

   q = g2[idx][b01];
   u = at2(rx0, ry1);
   q = g2[idx][b11];
   v = at2(rx1, ry1);
   b = lerp(sx, u, v);

   return lerp(sy, a, b);
}

double noise3(int idx, double vec[3])
{
   int bx0, bx1, by0, by1, bz0, bz1, b00, b10, b01, b11;
   double rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
   int i, j;
   /*
   if (start) {
      start = 0;
      init_perlin(time(0));
   }
*/

   setup(0, bx0, bx1, rx0, rx1);
   setup(1, by0, by1, ry0, ry1);
   setup(2, bz0, bz1, rz0, rz1);

   i = p[idx][bx0];
   j = p[idx][bx1];

   b00 = p[idx][i + by0];
   b10 = p[idx][j + by0];
   b01 = p[idx][i + by1];
   b11 = p[idx][j + by1];

   t = s_curve(rx0);
   sy = s_curve(ry0);
   sz = s_curve(rz0);

   q = g3[idx][b00 + bz0];
   u = at3(rx0, ry0, rz0);
   q = g3[idx][b10 + bz0];
   v = at3(rx1, ry0, rz0);
   a = lerp(t, u, v);

   q = g3[idx][b01 + bz0];
   u = at3(rx0, ry1, rz0);
   q = g3[idx][b11 + bz0];
   v = at3(rx1, ry1, rz0);
   b = lerp(t, u, v);

   c = lerp(sy, a, b);

   q = g3[idx][b00 + bz1];
   u = at3(rx0, ry0, rz1);
   q = g3[idx][b10 + bz1];
   v = at3(rx1, ry0, rz1);
   a = lerp(t, u, v);

   q = g3[idx][b01 + bz1];
   u = at3(rx0, ry1, rz1);
   q = g3[idx][b11 + bz1];
   v = at3(rx1, ry1, rz1);
   b = lerp(t, u, v);

   d = lerp(sy, a, b);

   return lerp(sz, c, d);
}

void normalize2(double v[2])
{
   double s;

   s = sqrt(v[0] * v[0] + v[1] * v[1]);
   v[0] = v[0] / s;
   v[1] = v[1] / s;
}

void normalize3(double v[3])
{
   double s;

   s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
   v[0] = v[0] / s;
   v[1] = v[1] / s;
   v[2] = v[2] / s;
}

void init_perlin(int idx, int seed)
{
   int i, j, k;

   /* Initialize the random number generator with a fixed
    * seed so that we get repeatable results! */
   srand(seed);

   for (i = 0; i < B; i++)
   {
      p[idx][i] = i;
      g1[idx][i] = (double)((random() % (B + B)) - B) / B;

      for (j = 0; j < 2; j++)
         g2[idx][i][j] = (double)((random() % (B + B)) - B) / B;

      normalize2(g2[idx][i]);

      for (j = 0; j < 3; j++)
         g3[idx][i][j] = (double)((random() % (B + B)) - B) / B;
      normalize3(g3[idx][i]);
   }

   while (--i)
   {
      k = p[idx][i];
      p[idx][i] = p[idx][j = random() % B];
      p[idx][j] = k;
   }

   for (i = 0; i < B + 2; i++)
   {
      p[idx][B + i] = p[idx][i];
      g1[idx][B + i] = g1[idx][i];
      for (j = 0; j < 2; j++)
         g2[idx][B + i][j] = g2[idx][i][j];
      for (j = 0; j < 3; j++)
         g3[idx][B + i][j] = g3[idx][i][j];
   }

   /* Reset the seed to something random in case we need it for other purposes. */
   srand(time(0));
}

/* --- My harmonic summing functions - PDB --------------------------*/

/*
   In what follows "alpha" is the weight when the sum is formed.
   Typically it is 2, As this approaches 1 the function is noisier.
   "beta" is the harmonic scaling/spacing, typically 2.
*/

double PerlinNoise1D(int idx, double x, double alpha, double beta, int n)
{
   int i;
   double val, sum = 0;
   double p, scale = 1;

   p = x;
   for (i = 0; i < n; i++)
   {
      val = noise1(idx, p);
      sum += val / scale;
      scale *= alpha;
      p *= beta;
   }
   return (sum);
}

/* scale = frequency, alpha = lacunarity, beta = gain */

double PerlinNoise2D(int idx, double x, double y, double alpha, double beta, int n)
{
   int i;
   double val, sum = 0;
   double p[2], scale = 1;

   p[0] = x;
   p[1] = y;
   for (i = 0; i < n; i++)
   {
      val = noise2(idx, p);
      //      val = ( val < 0 ? -val : val);
      sum += val / scale;
      scale *= alpha;
      p[0] *= beta;
      p[1] *= beta;
   }
   return (sum);
}

double PerlinNoise3D(int idx, double x, double y, double z, double alpha, double beta, int n)
{
   int i;
   double val, sum = 0;
   double p[3], scale = 1;

   p[0] = x;
   p[1] = y;
   p[2] = z;
   for (i = 0; i < n; i++)
   {
      val = noise3(idx, p);
      sum += val / scale;
      scale *= alpha;
      p[0] *= beta;
      p[1] *= beta;
      p[2] *= beta;
   }
   return (sum);
}

/* Ridged multifractal terrain model.
 *
 * Copyright 1994 F. Kenton Musgrave 
 *
 * Some good parameter values to start with:
 *
 *      H:           1.0
 *      offset:      1.0
 *      gain:        2.0
 */
double
RidgedMultifractal2D(int idx, double x, double y, double H, double lacunarity,
                     double octaves, double offset, double gain)
{
   double result, frequency, signal, weight, p[2];
   int i;
   static int first = 1;
   static double *exponent_array;

   p[0] = x;
   p[1] = y;

   /* precompute and store spectral weights */
   if (first)
   {
      /* seize required memory for exponent_array */
      exponent_array =
          (double *)malloc((octaves + 1) * sizeof(double));
      frequency = 1.0;
      for (i = 0; i <= octaves; i++)
      {
         /* compute weight for each frequency */
         exponent_array[i] = pow(frequency, -H);
         frequency *= lacunarity;
      }
      first = 0;
   }

   /* get first octave */
   signal = noise2(idx, p);
   /* get absolute value of signal (this creates the ridges) */
   if (signal < 0.0)
      signal = -signal;
   /* invert and translate (note that "offset" should be ~= 1.0) */
   signal = offset - signal;
   /* square the signal, to increase "sharpness" of ridges */
   signal *= signal;
   /* assign initial values */
   result = signal;
   weight = 1.0;

   for (i = 1; i < octaves; i++)
   {
      /* increase the frequency */
      p[0] *= lacunarity;
      p[1] *= lacunarity;

      /* weight successive contributions by previous signal */
      weight = signal * gain;
      if (weight > 1.0)
         weight = 1.0;
      if (weight < 0.0)
         weight = 0.0;
      signal = noise2(idx, p);
      if (signal < 0.0)
         signal = -signal;
      signal = offset - signal;
      signal *= signal;
      /* weight the contribution */
      signal *= weight;
      result += signal * exponent_array[i];
   }

   return (result);

} /* RidgedMultifractal() */
