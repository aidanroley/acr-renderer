#version 430 core
out vec4 FragColor;

in vec3 localPos;

uniform samplerCube environmentMap; // HDR cubemap

const float PI = 3.1415926;

void main() {

	vec3 normal = normalize(localPos);
	vec3 irradiance = vec3(0.0);

	vec3 envColor = texture(environmentMap, localPos).rgb;
	envColor = envColor / (envColor + vec3(1.0));
	envColor = pow(envColor, vec3(1.0/2.2));

	FragColor = vec4(envColor, 1.0);
}