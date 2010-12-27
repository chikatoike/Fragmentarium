#info Icosahedron Distance Estimator (Syntopia 2010)
#include "DE-Raytracer.frag"
#group Dodecahedron

// Based on Knighty's Kaleidoscopic IFS 3D Fractals, described here:
// http://www.fractalforums.com/3d-fractal-generation/kaleidoscopic-%28escape-time-ifs%29/

uniform float Scale; slider[0.00,2.617924,4.00]

// Return rotation matrix for rotating around vector v by angle
mat3  rotationMatrix3(vec3 v, float angle)
{
	float c = cos(radians(angle));
	float s = sin(radians(angle));
	
	return mat3(c + (1.0 - c) * v.x * v.x, (1.0 - c) * v.x * v.y - s * v.z, (1.0 - c) * v.x * v.z + s * v.y,
		(1.0 - c) * v.x * v.y + s * v.z, c + (1.0 - c) * v.y * v.y, (1.0 - c) * v.y * v.z - s * v.x,
		(1.0 - c) * v.x * v.z - s * v.y, (1.0 - c) * v.y * v.z + s * v.x, c + (1.0 - c) * v.z * v.z
		);
}


uniform float phi; slider[-5,1.618,5]

uniform float bailout; slider[4,9,12]
float bailout2 = pow(10.0,bailout);

vec3 n1 = normalize(vec3(-1.0,phi-1.0,1.0/(phi-1.0)));
vec3 n2 = normalize(vec3(phi-1.0,1.0/(phi-1.0),-1.0));
vec3 n3 = normalize(vec3(1.0/(phi-1.0),-1.0,phi-1.0));
       

vec3 offset = vec3(1.0,1.0,1.0);

uniform float Angle1; slider[-180,0,180]
uniform vec3 Rot1; slider[(-1,-1,-1),(1,1,1),(1,1,1)]
uniform float Angle2; slider[-180,0,180]
uniform vec3 Rot2; slider[(-1,-1,-1),(1,1,1),(1,1,1)]


mat3 fracRotation2 = rotationMatrix3(normalize(Rot2), Angle2);
mat3 fracRotation1 = rotationMatrix3(normalize(Rot1), Angle1);

// Number of fractal iterations.
uniform int Iterations;  slider[0,13,100]




float DE(vec3 z)
{
	float r;
	
	// Prefolds.
	float t;
	// Iterate to compute the distance estimator.
	int n = 0;
	while (n < Iterations) {
		z *= fracRotation1;
		t =dot(z,n1); if (t<0.0) { z-=2.0*t*n1; }
		t =dot(z,n2); if (t<0.0) { z-=2.0*t*n2; }
		t =dot(z,n3); if (t<0.0) { z-=2.0*t*n3; }
		t =dot(z,n1); if (t<0.0) { z-=2.0*t*n1; }
		t =dot(z,n2); if (t<0.0) { z-=2.0*t*n2; }
		t =dot(z,n3); if (t<0.0) { z-=2.0*t*n3; }
		t =dot(z,n1); if (t<0.0) { z-=2.0*t*n1; }
		t =dot(z,n2); if (t<0.0) { z-=2.0*t*n2; }
		t =dot(z,n3); if (t<0.0) { z-=2.0*t*n3; }
		z = z*Scale - offset*(Scale-1.0);
		z *= fracRotation2;
		
		r = dot(z, z);
		if (r > bailout2) break;
		n++;
	}
	
       // Works better when subtracting -1
	return (length(z) ) * pow(Scale,  float(-n-1));
}