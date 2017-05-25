#version 150 core

uniform vec3  ambientColour;
uniform vec3  invLightDir;			//Directional Light
uniform vec3  cameraPos;
uniform float specularIntensity;

in Vertex	{
	vec3 colour;
	vec3 worldPos;
	vec3 normal;
} IN;

out vec4 gl_FragColor;

void main(void)	{
//Lighting Calculations
	vec3 normal 		= normalize(IN.normal);
	vec3 viewDir 		= normalize(cameraPos - IN.worldPos );
	vec3 halfDir 		= normalize(invLightDir + viewDir );
	float rFactor       = max(0.0, dot(halfDir , normal ));
	
	float dFactor       = max(0.0, dot(invLightDir , normal )) ;
    float sFactor       = pow(rFactor , specularIntensity );
	   
//Colour Computations
	vec3 diffColour = IN.colour;
	vec3 specColour = min(diffColour + vec3(0.5f), vec3(1)); //Quick hack to approximate specular colour of an object, assuming the light colour is white
    vec3 col = ambientColour * diffColour 
			 + diffColour * dFactor
			 + specColour * sFactor * 0.33f;
		
//Output Final Fragment Colour
	gl_FragColor.rgb 	= col;
	gl_FragColor.a 		= 1.0;
}