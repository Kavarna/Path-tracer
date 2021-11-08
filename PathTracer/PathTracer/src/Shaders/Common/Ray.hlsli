
#ifndef _RAY_HLSLI_
#define _RAY_HLSLI_

struct Ray {
	
	float3 position;
	float3 direction;
    float length;
};

// Maybe modify this with the scene?
#define MAXIMUM_RAY_LENGTH 10000.f

#endif // _RAY_HLSLI_
