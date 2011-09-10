#info Menger Distance Estimator.
#include "DE-Raytracer.frag"
#include "MathUtils.frag"
#group Menger
// Based on Knighty's Kaleidoscopic IFS 3D Fractals, described here:
// http://www.fractalforums.com/3d-fractal-generation/kaleidoscopic-%28escape-time-ifs%29/

// Scale parameter. A perfect Menger is 3.0
uniform float Scale; slider[-5.00,2.0,4.00]


// Scaling center
uniform vec3 Offset; slider[(0,0,0),(1,1,1),(5,5,5)]

mat3 rot;


uniform float fixedRadius2; slider[0.1,1.0,2.3]
uniform float minRadius2; slider[0.0,0.25,2.3]
void sphereFold(inout vec3 z, inout float dz) {
	float r2 = dot(z,z);
	if (r2< minRadius2) {
		float temp = (fixedRadius2/minRadius2);
		z*= temp;
		dz*=temp;
} else if (r2<fixedRadius2) {
		float temp =(fixedRadius2/r2);
		z*=temp;
		dz*=temp;
	}
}

uniform float foldingValue; slider[0.0,2.0,5.0]
uniform float foldingLimit; slider[0.0,1.0,5.0]
void boxFold2(inout vec3 z, inout float dz) {
	if (z.x>foldingLimit) { z.x = foldingValue-z.x; }  else if (z.x<-foldingLimit) z.x = -foldingValue-z.x;
	if (z.y>foldingLimit)  { z.y = foldingValue-z.y;  } else if (z.y<-foldingLimit) z.y = -foldingValue-z.y;
	if (z.z>foldingLimit)  { z.z = foldingValue-z.z ; } else if (z.z<-foldingLimit) z.z = -foldingValue-z.z;
}

uniform float foldingLimit2; slider[0.0,1.0,5.0]
void boxFold(inout vec3 z, inout float dz) {
	z = clamp(z, -foldingLimit, foldingLimit) * 2.0 - z;
}

void boxFold3(inout vec3 z, inout float dz) {
	z = clamp(z, -foldingLimit2,foldingLimit2) * 2.0 - z;
}


void mengerFold(inout vec3 z, inout float dz) {
	z = abs(z);
	if (z.x<z.y){ z.xy = z.yx;}
	if (z.x< z.z){ z.xz = z.zx;}
	if (z.y<z.z){ z.yz = z.zy;}
	z = Scale*z-Offset*(Scale-1.0);
	if( z.z<-0.5*Offset.z*(Scale-1.0))  z.z+=Offset.z*(Scale-1.0);
	dz*=Scale;
	
}

uniform float Scale2; slider[0.00,2,4.00]
uniform vec3 Offset2; slider[(0,0,0),(1,0,0),(1,1,1)]

void octo(inout vec3 z, inout float dz) {	
		if (z.x+z.y<0.0) z.xy = -z.yx;
		if (z.x+z.z<0.0) z.xz = -z.zx;
		if (z.x-z.y<0.0) z.xy = z.yx;
		if (z.x-z.z<0.0) z.xz = z.zx;
		z = z*Scale2 - Offset2*(Scale2-1.0);
    dz*= Scale2;
}

#preset
Offset = 1.25,0.8456,0.9191
fixedRadius2 = 1
minRadius2 = 0.0625
Scale=-2
foldingValue = 2
#endpreset

uniform float Power; slider[0.1,2.0,12.3]
uniform float ZMUL; slider[0.0,1,310]
void powN2(inout vec3 z, float zr0, inout float dr) {
	float zo0 = asin( z.z/zr0 );
	float zi0 = atan( z.y,z.x );
	float zr = pow( zr0, Power-1.0 );
	float zo = zo0 * Power;
	float zi = zi0 * Power;
	dr = zr*dr*Power*abs(length(vec3(1.0,1.0,ZMUL)/sqrt(3.0))) + 1.0;
	zr *= zr0;
	z  = zr*vec3( cos(zo)*cos(zi), cos(zo)*sin(zi), ZMUL*sin(zo) );
}



float DE2(vec3 pos) {
	vec3 z=pos;
	float r;
	float dr=1.0;
	int i=0;
	r=length(z);
	while(r<100 && (i<8)) {
		powN2(z,r,dr);
		z+=pos;
		r=length(z);
		z*=rot;
		if (i<5) orbitTrap = min(orbitTrap, abs(vec4(z.x,z.y,z.z,r*r)));
		i++;
	}
	
	return 0.5*log(r)*r/dr;
	
}
uniform int MN; slider[0,5,50]
float DE(vec3 z, inout float dz, inout int iter)
{
	vec3 c = z;
	// z = vec3(0.0);
	int n = 0;
	//float dz = 1.0;
	float r = length(z);
	while (r<10 && n < 14) {
		if (n==iter)break;
		
if (n<MN) {
		boxFold(z,dz);
		sphereFold(z,dz);
		z = Scale*z; //+c;//+c*Offset;
		dz*=abs(Scale);
		} else {
		boxFold3(z,dz);
			r = length(z);
		
powN2(z,r,dz);

//z = z + c;
	
}
//		boxFold(z,dz);
		//r = length(z);		
//sphereFold(z,dz);
//octo(z,dz);
//		boxFold(z,dz);
	
//mengerFold(z,dz);
	
		
	//	z+=c*Offset;
		r = length(z);
		
		if (n<2 && iter<0) orbitTrap = min(orbitTrap, (vec4(abs(4.0*z),dot(z,z))));
		n++;
	}
	if (iter<0) iter = n;
	
	return r; // (r*log(r) / dz);
}
uniform bool Analytic; checkbox[true]

uniform float DetailGrad;slider[-7,-2.8,7];
float gradEPS = pow(10.0,DetailGrad);

float DE(vec3 pos) {
	int iter = -1;
	float dz = 1.0;
	if (Analytic) {
		float r = DE(pos, dz, iter);
		return (r*log(r) / dz);
	} else  {
		vec3 e = vec3(0.0,gradEPS,0.0);
		float r = abs(DE(pos, dz, iter));
		vec3 grad =vec3( DE(pos+e.yxx, dz, iter),  DE(pos+e.xyx, dz, iter),  DE(pos+e.xxy,dz,  iter) )-vec3(r);
		return r*log(r)*0.5/ length( grad/gradEPS);
	}
	
	
}

float DED(vec3 pos, vec3 dir) {
	int iter = -1;
	float dz = 1.0;
	vec3 e = -dir*gradEPS;
	float r = abs(DE(pos, dz, iter));
	float grad =DE(pos+e, dz, iter)-r;
	return r*log(r)*0.5/ abs( grad/gradEPS);
	
	
}

#preset A1
FOV = 0.832
Eye = -3.53574,3.53255,-3.78617
Target = -5.57431,5.66653,-6.87778
Up = 0.497594,-0.523851,-0.689695
AntiAlias = 1
AntiAliasBlur = 1
Detail = -3.95654
MaxStep = -0.43
DetailNormal = -3.15854
DetailAO = -1.05
FudgeFactor = 0.09412
MaxRaySteps = 1195
MaxRayStepsDiv = 2.5861
BoundingSphere = 9.1803
Dither = 0.5
AO = 0,0,0,0.7
Specular = 1.5306
SpecularExp = 16
SpotLight = 1,1,1,0.4
SpotLightDir = -0.4217,0.1
CamLight = 1,1,1,1.38028
CamLightMin = 0.18824
Glow = 1,0,0,0
Fog = 0.88188
HardShadow = 1
BaseColor = 1,1,1
OrbitStrength = 0.5443
X = 0.5,0.6,0.6,-0.9238
Y = 1,0.6,0,0.44762
Z = 0.8,0.78,1,0.02858
R = 0.654902,0.647059,0.666667,1
BackgroundColor = 0.6,0.6,0.45
GradientBackground = 0.3
CycleColors = false
Cycles = 3.97527
Scale = -1.05143
Offset = 1,1,1
fixedRadius2 = 0.84217
minRadius2 = 0
foldingValue = 2
foldingLimit = 1.7935
foldingLimit2 = 1.1628
Scale2 = 2
Offset2 = 1,0,0
Power = 2.00247
MN = 11
Analytic = true
DetailGrad = -1.0654
ZMUL = 20.2802
#endpreset

#preset Futura
FOV = 0.832
Eye = -4.04687,2.26031,-3.46749
Target = -4.79811,5.81644,-5.39479
Up = 0.571915,-0.294996,-0.763923
AntiAlias = 1
AntiAliasBlur = 1
Detail = -4.01737
MaxStep = -0.43
DetailNormal = -3.07314
DetailAO = -1.19
FudgeFactor = 0.09412
MaxRaySteps = 1195
MaxRayStepsDiv = 2.5861
BoundingSphere = 10
Dither = 0.5
AO = 0,0,0,0.7
Specular = 1.5306
SpecularExp = 16
SpotLight = 1,1,1,0.4
SpotLightDir = -0.4217,0.1
CamLight = 1,1,1,1.38028
CamLightMin = 0.18824
Glow = 1,0,0,0
Fog = 0.31496
HardShadow = 1
BaseColor = 1,1,1
OrbitStrength = 0.5443
X = 0.5,0.6,0.6,-0.9238
Y = 1,0.6,0,0.44762
Z = 0.8,0.78,1,0.02858
R = 0.654902,0.647059,0.666667,1
BackgroundColor = 0.6,0.6,0.45
GradientBackground = 0.3
CycleColors = false
Cycles = 3.97527
Scale = -1.05143
Offset = 1,1,1
fixedRadius2 = 0.76264
minRadius2 = 0.85935
foldingValue = 2
foldingLimit = 0.76085
foldingLimit2 = 2.61625
Scale2 = 2
Offset2 = 1,0,0
Power = 2.00247
MN = 11
Analytic = true
DetailGrad = -1.0654
ZMUL = 46.3543
#endpreset

#preset p1
FOV = 0.595581
Eye = 1.25125,-0.200486,1.00756
Target = -6.83342,0.316279,4.0991
Up = 0.328927,-0.0522827,0.868915
AntiAlias = 1
AntiAliasBlur = 1
Detail = -2.60582
DetailNormal = -2.42305
DetailAO = 0
AOSpread = 1
FudgeFactor = 0.26168
MaxRaySteps = 652
MaxRayStepsDiv = 2
BoundingSphere = 2
Dither = 0.5
AO = 0,0,0,0.81967
Specular = 0.4167
SpecularExp = 16
SpotLight = 1,1,1,0.72826
SpotLightDir = 0.54286,0.1
CamLight = 1,1,1,0.23656
Glow = 0.356863,1,0.12549,0.50876
Fog = 0.10738
HardShadow = 1
BaseColor = 1,1,1
OrbitStrength = 0.42574
X = 0.411765,0.6,0.560784,0.88976
Y = 0.666667,0.666667,0.498039,0.1496
Z = 1,0,0,0.46456
R = 1,0.666667,0,0.60318
BackgroundColor = 0.203922,0.227451,0.368627
GradientBackground = 0.5
CycleColors = true
Cycles = 6.78762
Scale = 2
RotVector = 1,1,1
RotAngle = 0
Offset = 1,1,1
fixedRadius2 = 1
minRadius2 = 0.25
foldingValue = 2
foldingLimit = 2.10525
ZMUL = -0.229
Analytic = false
DetailGrad = -3.56146
#endpreset

#preset F
FOV = 0.37038
Eye = 0.118532,-0.622267,-0.684692
Target = 7.08645,2.91063,1.83782
Up = 0.225292,0.185859,-0.882628
AntiAlias = 1
AntiAliasBlur = 1
Detail = -3.3572
DetailNormal = -3.98461
DetailAO = 0
AOSpread = 1
FudgeFactor = 0.2243
MaxRaySteps = 803
MaxRayStepsDiv = 2
BoundingSphere = 7.2289
Dither = 0.5
AO = 0,0,0,1
Specular = 4.1974
SpecularExp = 16
SpotLight = 1,1,1,0.5283
SpotLightDir = -0.15152,0.0303
CamLight = 1,1,1,0.33334
Glow = 0.356863,1,0.12549,0.10667
Fog = 0.9091
HardShadow = 0.95522
BaseColor = 1,1,1
OrbitStrength = 0.91935
X = 0.411765,0.6,0.560784,0.38582
Y = 0.666667,0.666667,0.498039,0.3409
Z = 1,0,0,0.93182
R = 1,0.666667,0,0.72414
BackgroundColor = 0.203922,0.227451,0.368627
GradientBackground = 1.07145
CycleColors = true
Cycles = 10.7147
Scale = 2
RotVector = 1,1,1
RotAngle = 0
Offset = 0,0.8763,1.7526
fixedRadius2 = 1.36665
minRadius2 = 0.96352
foldingValue = 2
foldingLimit = 3.53335
ZMUL = -1.087
Analytic = true
DetailGrad = -3.56146
Power = 12.3
#endpreset

#preset Ville
FOV = 0.62536
Eye = 4.36274,2.40381,0.932371
Target = -2.8913,-0.351315,0.36855
Up = 0.0793765,-0.00854807,-0.979476
AntiAlias = 1
AntiAliasBlur = 1
Detail = -3.06572
DetailNormal = -4.03844
DetailAO = 0
FudgeFactor = 0.04673
MaxRaySteps = 583
MaxRayStepsDiv = 1.8
BoundingSphere = 10
Dither = 0.5
AO = 0,0,0,0.7
Specular = 2.4348
SpecularExp = 16
SpotLight = 1,1,1,0.73563
SpotLightDir = -0.52,0.1
CamLight = 1,1,1,0.77273
CamLightMin = 0.18692
Glow = 1,1,1,0
Fog = 0.24162
HardShadow = 1
BaseColor = 1,1,1
OrbitStrength = 0.16832
X = 0.411765,0.6,0.560784,0.30708
Y = 0.666667,0.666667,0.498039,0.40158
Z = 0.666667,0.333333,1,0.02362
R = 0.4,0.7,1,-0.96832
BackgroundColor = 0.6,0.6,0.45
GradientBackground = 0.3
CycleColors = false
Cycles = 18.1816
Scale = -2
RotVector = 1,1,1
RotAngle = 0
Offset = 1.25,0.8456,0.9191
fixedRadius2 = 1
minRadius2 = 0.0625
foldingValue = 2
foldingLimit = 2.01755
Scale2 = 2
Offset2 = 1,0,0
Power = 2.05554
ZMUL = -9.084
Analytic = true
DetailGrad = -2.8
#endpreset

#preset
FOV = 0.62536
Eye = -3.96357,2.54298,3.35567
Target = 2.65061,1.73656,2.28704
Up = 0.153672,-0.0460469,0.985891
AntiAlias = 1
AntiAliasBlur = 1
Detail = -5.05918
DetailNormal = -5.18357
DetailAO = 0
FudgeFactor = 0.01073
MaxRaySteps = 426
MaxRayStepsDiv = 1
BoundingSphere = 10
Dither = 0.5
AO = 0,0,0,0.93443
Specular = 3.4167
SpecularExp = 21.875
SpotLight = 1,1,1,0.42391
SpotLightDir = 1,0.1
CamLight = 1,1,1,0.51612
CamLightMin = 0.53271
Glow = 0.364706,0,1,0.12281
Fog = 0.42954
HardShadow = 0.37736
BaseColor = 0.666667,0.666667,0.498039
OrbitStrength = 0.13861
X = 0.6,0.439216,0.32549,0.16536
Y = 0.666667,0.666667,0,0.2756
Z = 1,0,0.0156863,0.16536
R = 1,0.490196,0.235294,-0.3651
BackgroundColor = 0.6,0.6,0.45
GradientBackground = 0.3005
CycleColors = true
Cycles = 7.45609
Scale = -2.00012
Offset = 1.25,0.8456,0.9191
fixedRadius2 = 1.02191
minRadius2 = 0.06631
foldingValue = 2
foldingLimit = 1
foldingLimit2 = 2
Scale2 = 2
Offset2 = 1,0,0
Power = 2.05493
ZMUL = -26
MN = 6
Analytic = true
DetailGrad = -5.40316
#endpreset