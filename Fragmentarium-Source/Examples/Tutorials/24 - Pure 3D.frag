#include "3D.frag"
uniform sampler2D texture; file[f:\pano.jpg]
uniform sampler2D texture2; file[f:\pano3.jpg]

// An example of working with 3D without a distance estimator

// Free HDRI's: http://www.hdrlabs.com/sibl/archive.html

float sphereLine(vec3 pos, vec3 dir, vec3 center, float radius, out vec3 normal) {
	vec3 sd=center-pos;
	float b = dot(dir,sd);
	float temp = b*b+radius*radius-dot(sd,sd);
	if (temp>0.0) {
		temp = b - sqrt(temp); // intersection distance
		normal = normalize((pos+temp*dir)-center);
		return temp;
	}
	return -1.;
}

#define PI  3.14159265358979323846264

vec2 spherical(vec3 dir) {
	return vec2( acos(dir.z)/PI, atan(dir.y,dir.x)/(2.0*PI) );
}

uniform vec2 Specular; slider[(0,0),(1,0),(1,0.1)]
uniform vec3 Diffuse;slider[(0,0,0),(1,0,1),(1,0.1,10)]

vec3 color(vec3 pos, vec3 dir) {
	
	vec3 normal = vec3(1.0,1.0,1.0);
	float intersect = 10000.0;
	vec3 finalNormal;
	for (int i = 0; i < 6; i++) {
	float z = (float(i)-6.0)/2.0;
		float i = sphereLine(pos,normalize(dir),vec3(1.0,0.0+float(i),z),0.5, normal);
		if (i<intersect && i>0.) {
			intersect =i;
			finalNormal = normal;
		}
	}
	
	if (intersect<10000.) {

vec2 f = rand2(viewCoord*(float(backbufferCounter)+1.0))-vec2(0.5);

		vec3 reflected = -2.0*dot(dir,finalNormal)*finalNormal+dir;
	    //  vec3 reflectedP = reflected + 
// Angle of perturbated vector
// (cos(phi1)*cos(phi2)*cos(theta2 - theta1) + sin(phi1)*sin(phi2))		
// Theta wil be y value (after indices are shuffled)
		vec3 col = Specular.x*texture2D(texture,spherical(normalize(reflected)).yx+f*Specular.y).xyz;
		vec3 col2 = Diffuse.x*texture2D(texture2,spherical(normalize(finalNormal)).yx+f*Diffuse.y).xyz;
		col2 = pow(col2,Diffuse.z);
		return col+col2;
	}
	vec2 c = spherical(normalize(dir));
	vec3 col = texture2D(texture,c.yx).xyz;
	
	return col;
}