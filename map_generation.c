#include<stdlib.h>
#include<stdio.h>
#include<math.h>
  
double fade(double t){
    return t*t*t*(t*(t*6-15)+10);
} 

double lerp(double t, double a, double b){
    return a + t*(b - a); 
}

double grad(int hash, double x, double y, double z) {
    int h = hash & 15;                      
    double u = h<8 ? x : y,                 
        v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

double noise(double x, double y, double z) {
    int permutation[] = {151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180};

    int p[512];

    for (int i = 0; i < 256; i++){
        p[256+i] = p[i] = permutation[i];
    }
    
    int X = (int)floor(x) & 255, Y = (int)floor(y) & 255, Z = (int)floor(z) & 255;
    // Find unit grid cell containing point 
    // Get relative xyz coordinates of point within that cell 
    x -= floor(x);                                
    y -= floor(y);                                
    z -= floor(z);

    double u = fade(x), v = fade(y), w = fade(z);
    int A = p[X  ]+Y, AA = p[A]+Z, AB = p[A+1]+Z, B = p[X+1]+Y, BA = p[B]+Z, BB = p[B+1]+Z;  
    return lerp(w, lerp(v, lerp(u, grad(p[AA  ], x  , y  , z   ), 
                                    grad(p[BA  ], x-1, y  , z   )),
                            lerp(u, grad(p[AB  ], x  , y-1, z   ), 
                                    grad(p[BB  ], x-1, y-1, z   ))),
                    lerp(v, lerp(u, grad(p[AA+1], x  , y  , z-1 ), 
                                    grad(p[BA+1], x-1, y  , z-1 )), 
                            lerp(u, grad(p[AB+1], x  , y-1, z-1 ),
                                    grad(p[BB+1], x-1, y-1, z-1 ))));
}

int generate_terrain (int size, float z_layer, float **z) {
    // Scaling Factors 
    float scaling[] = {0.25, 0.5, 1, 1.5, 2, 2.5, 3, 3.5};
    int octaves = 8;

    // Fill array
    for(int x = 0; x < size; x++) {
        float x_noise = x/(float)size*4;
        for(int y = 0; y < size; y++) {
            float y_noise = y/(float)size*4;
            //Adding Altitudes for different frequencies 
            double height = 0;
            for(int i = 0; i < octaves; i++) {
                height += noise(x_noise*scaling[i]+pow(1 + scaling[i], 1.5), y_noise*scaling[i]+pow(1 + scaling[i], 1.5),z_layer)/pow(1 + scaling[i], 0.25);   
            }  
            z[x][y] = height;        
        }
    }
    return 0;
}
