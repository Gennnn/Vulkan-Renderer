#pragma once

struct float2 {
	union {
		struct {
			float x;
			float y;
		};
		struct {
			float u;
			float v;
		};
		float xy[2];
	};
};

struct float3 {
	union {
		struct {
			float x;
			float y;
			float z;
		};
		struct {
			float r;
			float g;
			float b;
		};
		float xyz[3];
		float rgb[3];
	};
};

struct float4 {
	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct {
			float r;
			float g;
			float b;
			float a;
		};
		float rgba[4];
		float xyzw[4];
	};
};