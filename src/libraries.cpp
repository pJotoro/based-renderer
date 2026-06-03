#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// TODO: Add Findcgltf.cmake and remove cgltf.h
#define CGLTF_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4996)
#include "cgltf.h"
#pragma warning(pop)