//
// Copyright (c) 2013-2014 Samuel Villarreal
// svkaiser@gmail.com
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
 //   2. Altered source versions must be plainly marked as such, and must not be
 //   misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.
// 
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "surfaces.h"
#include "trace.h"
#include "mapData.h"
#include "lightmap.h"
#include "kexlib/binFile.h"

//#define EXPORT_TEXELS_OBJ

//
// kexLightmapBuilder::kexLightmapBuilder
//

kexLightmapBuilder::kexLightmapBuilder(void) {
    this->textureWidth  = 128;
    this->textureHeight = 128;
    this->allocBlocks   = NULL;
    this->numTextures   = 0;
    this->samples       = 16;
    this->extraSamples  = 2;
    this->ambience      = 0.0f;
    this->tracedTexels  = 0;
}

//
// kexLightmapBuilder::~kexLightmapBuilder
//

kexLightmapBuilder::~kexLightmapBuilder(void) {
}

//
// kexLightmapBuilder::AddThingLights
//

void kexLightmapBuilder::AddThingLights(kexDoomMap &doomMap) {
    mapThing_t *thing;
    int j;

    printf("------------- Adding thing lights -------------\n");

    for(int i = 0; i < doomMap.numThings; i++) {
        thing = &doomMap.mapThings[i];

        if(!(thing->type >= TYPE_LIGHTPOINT && thing->type <= NUMLIGHTTYPES)) {
            continue;
        }

        if(thing->type == TYPE_DIRECTIONAL_TARGET) {
            continue;
        }

        if(thing->angle == 0) {
            continue;
        }

        thing->options = 255;

        if(thing->tid != 0) {
            for(j = 0; j < doomMap.numSectors; j++) {
                if(doomMap.mapSectors[j].tag == thing->tid) {
                    thing->options = doomMap.mapSectors[j].colors[2];
                    break;
                }
            }
        }

        thingLights.Push(thing);
        printf(".");
    }

    lightInfos = doomMap.lightInfos;
    printf("\n");
}

//
// kexLightmapBuilder::NewTexture
//

void kexLightmapBuilder::NewTexture(void) {
    numTextures++;

    if(allocBlocks == NULL) {
        allocBlocks = (int*)Mem_Calloc(sizeof(int) * textureWidth, hb_static);
    }

    memset(allocBlocks, 0, sizeof(int) * textureWidth);

    byte *texture = (byte*)Mem_Calloc((textureWidth * textureHeight) * 3, hb_static);
    textures.Push(texture);
    currentTexture = texture;
}

//
// kexLightMapBuilder::MakeRoomForBlock
//

bool kexLightmapBuilder::MakeRoomForBlock(const int width, const int height,
                                          int *x, int *y) {
    int i;
    int j;
    int bestRow1;
    int bestRow2;

    if(allocBlocks == NULL) {
        return false;
    }

    bestRow1 = textureHeight;

    for(i = 0; i <= textureWidth - width; i++) {
        bestRow2 = 0;

        for(j = 0; j < width; j++) {
            if(allocBlocks[i + j] >= bestRow1) {
                break;
            }

            if(allocBlocks[i + j] > bestRow2) {
                bestRow2 = allocBlocks[i + j];
            }
        }

        // found a free block
        if(j == width) {
            *x = i;
            *y = bestRow1 = bestRow2;
        }
    }

    if(bestRow1 + height > textureHeight) {
        // no room
        return false;
    }

    for(i = 0; i < width; i++) {
        // store row offset
        allocBlocks[*x + i] = bestRow1 + height;
    }

    return true;
}

//
// kexLightmapBuilder::GetBoundsFromSurface
//

kexBBox kexLightmapBuilder::GetBoundsFromSurface(const surface_t *surface) {
    kexVec3 low(M_INFINITY, M_INFINITY, M_INFINITY);
    kexVec3 hi(-M_INFINITY, -M_INFINITY, -M_INFINITY);

    kexBBox bounds;
    bounds.Clear();

    for(int i = 0; i < surface->numVerts; i++) {
        for(int j = 0; j < 3; j++) {
            if(surface->verts[i][j] < low[j]) {
                low[j] = surface->verts[i][j];
            }
            if(surface->verts[i][j] > hi[j]) {
                hi[j] = surface->verts[i][j];
            }
        }
    }

    bounds.min = low;
    bounds.max = hi;

    return bounds;
}

//
// kexLightmapBuilder::EmitFromCeiling
//

bool kexLightmapBuilder::EmitFromCeiling(const kexVec3 &origin, const kexVec3 &normal,
                                         const mapThing_t *light, float *dist) {
    mapSubSector_t *sub;
    mapSubSector_t *tSub;
    mapSector_t *sector;
    surface_t *surf;
    kexVec3 dir;
    int num;
    int i;
    unsigned int k;

    if(!(sub = map->PointInSubSector(light->x, light->y))) {
        return false;
    }
    if(!(sector = map->GetSectorFromSubSector(sub))) {
        return false;
    }

    num = sub - map->mapSSects;
    surf = NULL;

    for(k = 0; k < surfaces.Length(); k++) {
        if(surfaces[k]->type != ST_CEILING) {
            continue;
        }

        if(surfaces[k]->typeIndex == num) {
            surf = surfaces[k];
            break;
        }
    }

    if(surf == NULL) {
        return false;
    }

    dir = kexVec3(0, 0, 1);

    for(i = 0; i < map->numThings; i++) {
        mapThing_t *thing = &map->mapThings[i];

        if(thing->type != TYPE_DIRECTIONAL_TARGET) {
            continue;
        }

        if(thing->tid != light->tid) {
            continue;
        }

        dir = kexVec3(F(light->x << 16), F(light->y << 16), F(light->z << 16)) -
            kexVec3(F(thing->x << 16), F(thing->y << 16), F(thing->z << 16));

        dir.Normalize();
        break;
    }

    *dist = normal.Dot(dir);

    if(*dist <= 0) {
        return false;
    }

    trace.Trace(origin, origin + (dir * 32768));

    if(trace.fraction == 1 || trace.hitSurface == NULL) {
        return false;
    }

    if(trace.hitSurface->type != ST_CEILING) {
        return false;
    }

    tSub = &map->mapSSects[trace.hitSurface->typeIndex];

    if(map->GetSectorFromSubSector(tSub) != sector ||
        trace.hitSurface->type != ST_CEILING) {
            return false;
    }

    return true;
}

//
// kexLightmapBuilder::LightTexelSample
//

kexVec3 kexLightmapBuilder::LightTexelSample(const kexVec3 &origin, kexPlane &plane) {
    mapThing_t *light;
    kexVec3 lightOrigin;
    kexVec3 dir;
    kexVec3 color;
    int j;
    float dist;
    float radius;
    float intensity;
    float colorAdd;
    mapLightInfo_t *lInfo;
    mapLightInfo_t lInfo2;

    color.Clear();

    for(unsigned int i = 0; i < thingLights.Length(); i++) {
        light = thingLights[i];
        lightOrigin.Set(F(light->x << 16), F(light->y << 16), F(light->z << 16));

        if(ambience > 0.0f) {
            for(j = 0; j < 3; j++) {
                color[j] += ambience;
                if(color[j] > 1.0f) color[j] = 1.0f;
                if(color[j] < 0.0f) color[j] = 0.0f;
            }
        }

        if(plane.Distance(lightOrigin) - plane.d < 0) {
            continue;
        }

        radius = 50.0f * light->angle;

        if(light->options <= 255) {
            lInfo2.rgba[0] = (byte)light->options;
            lInfo2.rgba[1] = (byte)light->options;
            lInfo2.rgba[2] = (byte)light->options;
            lInfo2.rgba[3] = 0;
            lInfo2.tag = 0;
        }
        else {
            lInfo2 = lightInfos[light->options - 256];
        }

        lInfo = &lInfo2;

        //intensity = lInfo->rgba[3];
        intensity = 8;

        if(intensity < 1.0f) {
            intensity = 1.0f;
        }

        if(light->type == TYPE_DIRECTIONAL_CEILING) {
            if(!EmitFromCeiling(origin, plane.Normal(), light, &dist)) {
                continue;
            }

            for(j = 0; j < 3; j++) {
                color[j] += (/*dist **/ ((float)lInfo->rgba[j] / 255.0f));
                if(color[j] > 1.0f) color[j] = 1.0f;
                if(color[j] < 0.0f) color[j] = 0.0f;
            }

            tracedTexels++;
            continue;
        }

        trace.Trace(lightOrigin, origin);

        if(trace.fraction != 1) {
            continue;
        }

        dir = (lightOrigin - origin);
        dist = dir.Unit();

        if(light->type == TYPE_LIGHTPOINT_WEAK) {
            if(dist < 128) {
                dist = 128;
            }
        }

        dir.Normalize();

        colorAdd = radius / (dist * dist) * plane.Normal().Dot(dir);

        for(j = 0; j < 3; j++) {
            color[j] += (colorAdd * ((float)lInfo->rgba[j] / 255.0f)) * intensity;
            if(color[j] > 1.0f) color[j] = 1.0f;
            if(color[j] < 0.0f) color[j] = 0.0f;
        }

        tracedTexels++;
    }

    return color;
}

//
// kexLightmapBuilder::BuildSurfaceParams
//

void kexLightmapBuilder::BuildSurfaceParams(surface_t *surface) {
    kexPlane *plane;
    kexBBox bounds;
    kexVec3 roundedSize;
    int i;
    lightmapAxis_t axis;
    kexVec3 tCoords[2];
    kexVec3 tNormal;
    kexVec3 tDelta;
    kexVec3 tOrigin;
    int width;
    int height;
    int x;
    int y;
    float d;

    plane = &surface->plane;
    bounds = GetBoundsFromSurface(surface);

    // round off dimentions
    for(i = 0; i < 3; i++) {
        bounds.min[i] = samples * kexMath::Floor(bounds.min[i] / samples);
        bounds.max[i] = samples * kexMath::Ceil(bounds.max[i] / samples);

        roundedSize[i] = (bounds.max[i] - bounds.min[i]) / samples + 1;
    }

    tCoords[0].Clear();
    tCoords[1].Clear();

    tNormal.Set(
        kexMath::Fabs(plane->a),
        kexMath::Fabs(plane->b),
        kexMath::Fabs(plane->c));

    // figure out what axis the plane lies on
    if(tNormal.x >= tNormal.y && tNormal.x >= tNormal.z) {
        axis = AXIS_YZ;
    }
    else if(tNormal.y >= tNormal.x && tNormal.y >= tNormal.z) {
        axis = AXIS_XZ;
    }
    else {
        axis = AXIS_XY;
    }

    switch(axis) {
        case AXIS_YZ:
            width = (int)roundedSize.y;
            height = (int)roundedSize.z;
            tCoords[0].y = 1.0f / samples;
            tCoords[1].z = 1.0f / samples;
            break;

        case AXIS_XZ:
            width = (int)roundedSize.x;
            height = (int)roundedSize.z;
            tCoords[0].x = 1.0f / samples;
            tCoords[1].z = 1.0f / samples;
            break;

        case AXIS_XY:
            width = (int)roundedSize.x;
            height = (int)roundedSize.y;
            tCoords[0].x = 1.0f / samples;
            tCoords[1].y = 1.0f / samples;
            break;
    }

    // clamp width
    if(width > textureWidth) {
        tCoords[0] *= ((float)textureWidth / (float)width);
        width = textureWidth;
    }

    // clamp height
    if(height > textureHeight) {
        tCoords[1] *= ((float)textureHeight / (float)height);
        height = textureHeight;
    }

    // make room for new lightmap block
    if(!MakeRoomForBlock(width, height, &x, &y)) {
        NewTexture();
        if(!MakeRoomForBlock(width, height, &x, &y)) {
            Error("Lightmap allocation failed\n");
            return;
        }
    }

    surface->lightmapCoords = (float*)Mem_Calloc(sizeof(float) *
        surface->numVerts * 2, hb_static);

    // calculate texture coordinates
    for(i = 0; i < surface->numVerts; i++) {
        tDelta = surface->verts[i] - bounds.min;
        surface->lightmapCoords[i * 2 + 0] =
            (tDelta.Dot(tCoords[0]) + x + 0.5f) / (float)textureWidth;
        surface->lightmapCoords[i * 2 + 1] =
            (tDelta.Dot(tCoords[1]) + y + 0.5f) / (float)textureHeight;
    }

    tOrigin = bounds.min;

    // project tOrigin and tCoords so they lie on the plane
    d = (plane->Distance(bounds.min) - plane->d) / plane->Normal()[axis];
    tOrigin[axis] -= d;

    for(i = 0; i < 2; i++) {
        tCoords[i].Normalize();
        d = plane->Distance(tCoords[i]) / plane->Normal()[axis];
        tCoords[i][axis] -= d;
    }

    surface->lightmapNum = numTextures - 1;
    surface->lightmapDims[0] = width;
    surface->lightmapDims[1] = height;
    surface->lightmapOffs[0] = x;
    surface->lightmapOffs[1] = y;
    surface->lightmapOrigin = tOrigin;
    surface->lightmapSteps[0] = tCoords[0] * (float)samples;
    surface->lightmapSteps[1] = tCoords[1] * (float)samples;
}

//
// kexLightmapBuilder::TraceSurface
//

void kexLightmapBuilder::TraceSurface(surface_t *surface) {
    static kexVec3 colorSamples[LIGHTMAP_MAX_SIZE][LIGHTMAP_MAX_SIZE];
    int sampleWidth;
    int sampleHeight;
    kexVec3 normal;
    kexVec3 pos;
    int i;
    int j;

    memset(colorSamples, 0, sizeof(colorSamples));

    sampleWidth = surface->lightmapDims[0];
    sampleHeight = surface->lightmapDims[1];

    normal = surface->plane.Normal();

#ifdef EXPORT_TEXELS_OBJ
    static int cnt = 0;
    FILE *f = fopen(Va("texels_%02d.obj", cnt++), "w");
    int indices = 0;
#endif

    for(i = 0; i < sampleHeight; i++) {
        for(j = 0; j < sampleWidth; j++) {
            pos = surface->lightmapOrigin + normal +
                (surface->lightmapSteps[0] * (float)j) +
                (surface->lightmapSteps[1] * (float)i);

#ifdef EXPORT_TEXELS_OBJ
            ExportTexelsToObjFile(f, pos, indices);
            indices += 8;
#endif

            colorSamples[i][j] += LightTexelSample(pos, surface->plane);
        }
        printf(".");
    }

#ifdef EXPORT_TEXELS_OBJ
    fclose(f);
#endif

    for(i = 0; i < sampleHeight; i++) {
        for(j = 0; j < sampleWidth; j++) {
            int offs = (((textureWidth * (i + surface->lightmapOffs[1])) +
                surface->lightmapOffs[0]) * 3);

            currentTexture[offs + j * 3 + 0] = (byte)(colorSamples[i][j][2] * 255);
            currentTexture[offs + j * 3 + 1] = (byte)(colorSamples[i][j][1] * 255);
            currentTexture[offs + j * 3 + 2] = (byte)(colorSamples[i][j][0] * 255);
        }
        printf(".");
    }
}

//
// kexLightmapBuilder::CreateLightmaps
//

void kexLightmapBuilder::CreateLightmaps(kexDoomMap &doomMap) {
    trace.Init(doomMap);
    AddThingLights(doomMap);

    map = &doomMap;

    printf("------------- Building lightmap -------------\n");

    for(unsigned int i = 0; i < surfaces.Length(); i++) {
        printf("Lighting surface %03d: ", i);
        BuildSurfaceParams(surfaces[i]);
        TraceSurface(surfaces[i]);
        printf("\n");
    }

    printf("\nTexels traced: %i\n\n", tracedTexels);
}

//
// kexLightmapBuilder::CreateLightmapLump
//

byte *kexLightmapBuilder::CreateLightmapLump(int *size) {
    int lumpSize = 0;
    unsigned int i;
    int j;
    int numTexCoords;
    int coordOffsets;
    byte *data;
    kexBinFile lumpFile;
    fint_t fuv;

    // try to guess the actual lump size
    lumpSize += ((textureWidth * textureHeight) * 3) * textures.Length();
    lumpSize += (12 * surfaces.Length());
    lumpSize += 1024; // add some extra slop

    for(i = 0; i < surfaces.Length(); i++) {
        lumpSize += (surfaces[i]->numVerts * 2) * sizeof(short);
    }

    data = (byte*)Mem_Calloc(lumpSize, hb_static);
    lumpFile.SetBuffer(data);

    lumpFile.Write32(surfaces.Length());
    coordOffsets = 0;
    numTexCoords = 0;

    for(i = 0; i < surfaces.Length(); i++) {
        lumpFile.Write16(surfaces[i]->type);
        lumpFile.Write16(surfaces[i]->typeIndex);
        lumpFile.Write16(surfaces[i]->lightmapNum);
        lumpFile.Write16(surfaces[i]->numVerts * 2);
        lumpFile.Write32(coordOffsets);

        coordOffsets += (surfaces[i]->numVerts * 2);
        numTexCoords += surfaces[i]->numVerts * 2;
    }

    lumpFile.Write32(numTexCoords);

    for(i = 0; i < surfaces.Length(); i++) {
        for(j = 0; j < surfaces[i]->numVerts * 2; j++) {
            fuv.f = surfaces[i]->lightmapCoords[j];
            lumpFile.Write16(fuv.i >> 16);
        }
    }

    lumpFile.Write32(textures.Length());
    lumpFile.Write32(textureWidth);
    lumpFile.Write32(textureHeight);

    for(i = 0; i < textures.Length(); i++) {
        for(j = 0; j < (textureWidth * textureHeight) * 3; j++) {
            lumpFile.Write8(textures[i][j]);
        }
    }

    *size = lumpFile.BufferAt() - lumpFile.Buffer();
    return data;
}

//
// kexLightmapBuilder::WriteTexturesToTGA
//

void kexLightmapBuilder::WriteTexturesToTGA(void) {
    kexBinFile file;

    for(unsigned int i = 0; i < textures.Length(); i++) {
        file.Create(Va("lightmap_%02d.tga", i));
        file.Write16(0);
        file.Write16(2);
        file.Write16(0);
        file.Write16(0);
        file.Write16(0);
        file.Write16(0);
        file.Write16(textureWidth);
        file.Write16(textureHeight);
        file.Write16(24);

        for(int j = 0; j < (textureWidth * textureHeight) * 3; j++) {
            file.Write8(textures[i][j]);
        }
        file.Close();
    }
}

//
// kexLightmapBuilder::ExportTexelsToObjFile
//

void kexLightmapBuilder::ExportTexelsToObjFile(FILE *f, const kexVec3 &org, int indices) {
#define BLOCK(idx, offs) (org[idx] + offs)/256.0f
    fprintf(f, "o texel_%03d\n", indices);
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, -6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, 6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, 6), -BLOCK(0, -6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, -6), BLOCK(2, -6), -BLOCK(0, -6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, -6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, 6), -BLOCK(0, 6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, 6), -BLOCK(0, -6));
    fprintf(f, "v %f %f %f\n", -BLOCK(1, 6), BLOCK(2, -6), -BLOCK(0, -6));
    fprintf(f, "f %i %i %i %i\n", indices+1, indices+2, indices+3, indices+4);
    fprintf(f, "f %i %i %i %i\n", indices+5, indices+8, indices+7, indices+6);
    fprintf(f, "f %i %i %i %i\n", indices+1, indices+5, indices+6, indices+2);
    fprintf(f, "f %i %i %i %i\n", indices+2, indices+6, indices+7, indices+3);
    fprintf(f, "f %i %i %i %i\n", indices+3, indices+7, indices+8, indices+4);
    fprintf(f, "f %i %i %i %i\n\n", indices+5, indices+1, indices+4, indices+8);
#undef BLOCK
}
