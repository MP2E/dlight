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
// DESCRIPTION: Prepares geometry from map structures
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "surfaces.h"
#include "mapData.h"

//#define EXPORT_OBJ

kexArray<surface_t*> surfaces;

//
// Surface_AllocateFromSeg
//

static void Surface_AllocateFromSeg(kexDoomMap &doomMap, mapSeg_t *seg) {
    mapSideDef_t *side;
    surface_t *surf;
    float top, bTop;
    float bottom, bBottom;
    mapSector_t *front;
    mapSector_t *back;

    if(seg->linedef == -1) {
        return;
    }

    side = doomMap.GetSideDef(seg);
    front = doomMap.GetFrontSector(seg);
    back = doomMap.GetBackSector(seg);

    top = front->ceilingheight;
    bottom = front->floorheight;

    if(back) {
        bTop = back->ceilingheight;
        bBottom = back->floorheight;

        // bottom seg
        if(bottom < bBottom) {
            if(side->bottomtexture != -1) {
                surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
                surf->numVerts = 4;
                surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * 4, hb_static);

                surf->verts[0].x = surf->verts[2].x = F(doomMap.mapVerts[seg->v1].x);
                surf->verts[0].y = surf->verts[2].y = F(doomMap.mapVerts[seg->v1].y);
                surf->verts[1].x = surf->verts[3].x = F(doomMap.mapVerts[seg->v2].x);
                surf->verts[1].y = surf->verts[3].y = F(doomMap.mapVerts[seg->v2].y);
                surf->verts[0].z = surf->verts[1].z = bBottom;
                surf->verts[2].z = surf->verts[3].z = bottom;

                surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
                surf->plane.SetDistance(surf->verts[0]);
                surf->type = ST_LOWERSEG;
                surf->typeIndex = seg - doomMap.mapSegs;

                doomMap.segSurfaces[1][surf->typeIndex] = surf;

                surfaces.Push(surf);
            }

            bottom = bBottom;
        }

        // top seg
        if(top > bTop) {
            if(side->toptexture != -1) {
                surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
                surf->numVerts = 4;
                surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * 4, hb_static);

                surf->verts[0].x = surf->verts[2].x = F(doomMap.mapVerts[seg->v1].x);
                surf->verts[0].y = surf->verts[2].y = F(doomMap.mapVerts[seg->v1].y);
                surf->verts[1].x = surf->verts[3].x = F(doomMap.mapVerts[seg->v2].x);
                surf->verts[1].y = surf->verts[3].y = F(doomMap.mapVerts[seg->v2].y);
                surf->verts[0].z = surf->verts[1].z = top;
                surf->verts[2].z = surf->verts[3].z = bTop;

                surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
                surf->plane.SetDistance(surf->verts[0]);
                surf->type = ST_UPPERSEG;
                surf->typeIndex = seg - doomMap.mapSegs;

                doomMap.segSurfaces[2][surf->typeIndex] = surf;

                surfaces.Push(surf);
            }
            
            top = bTop;
        }
    }

    // middle seg
    if(side->midtexture != -1) {
        surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
        surf->numVerts = 4;
        surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * 4, hb_static);

        surf->verts[0].x = surf->verts[2].x = F(doomMap.mapVerts[seg->v1].x);
        surf->verts[0].y = surf->verts[2].y = F(doomMap.mapVerts[seg->v1].y);
        surf->verts[1].x = surf->verts[3].x = F(doomMap.mapVerts[seg->v2].x);
        surf->verts[1].y = surf->verts[3].y = F(doomMap.mapVerts[seg->v2].y);
        surf->verts[0].z = surf->verts[1].z = top;
        surf->verts[2].z = surf->verts[3].z = bottom;

        surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
        surf->plane.SetDistance(surf->verts[0]);
        surf->type = ST_MIDDLESEG;
        surf->typeIndex = seg - doomMap.mapSegs;

        doomMap.segSurfaces[0][surf->typeIndex] = surf;

        surfaces.Push(surf);
    }
}

//
// Surface_AllocateFromLeaf
//

static void Surface_AllocateFromLeaf(kexDoomMap &doomMap) {
    surface_t *surf;
    leaf_t *leaf;
    mapSector_t *sector = NULL;
    int i;
    int j;

    doomMap.leafSurfaces[0] = (surface_t**)Mem_Calloc(sizeof(surface_t*) *
        doomMap.numSSects, hb_static);
    doomMap.leafSurfaces[1] = (surface_t**)Mem_Calloc(sizeof(surface_t*) *
        doomMap.numSSects, hb_static);

    for(i = 0; i < doomMap.numSSects; i++) {
        if(doomMap.ssLeafCount[i] < 3) {
            continue;
        }

        for(j = 0; j < doomMap.mapSSects[i].numsegs; j++) {
            mapSeg_t *seg = &doomMap.mapSegs[doomMap.mapSSects[i].firstseg + j];
            if(seg->side != -1) {
                sector = doomMap.GetFrontSector(seg);
                break;
            }
        }

        surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
        surf->numVerts = doomMap.ssLeafCount[i];
        surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * surf->numVerts, hb_static);

        // floor verts
        for(j = 0; j < surf->numVerts; j++) {
            leaf = &doomMap.leafs[doomMap.ssLeafLookup[i] + j];

            surf->verts[j].x = F(leaf->vertex->x);
            surf->verts[j].y = F(leaf->vertex->y);
            surf->verts[j].z = sector->floorheight;
        }

        surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
        surf->plane.SetDistance(surf->verts[0]);
        surf->type = ST_FLOOR;
        surf->typeIndex = i;

        doomMap.leafSurfaces[0][i] = surf;

        surfaces.Push(surf);

        surf = (surface_t*)Mem_Calloc(sizeof(surface_t), hb_static);
        surf->numVerts = doomMap.ssLeafCount[i];
        surf->verts = (kexVec3*)Mem_Calloc(sizeof(kexVec3) * surf->numVerts, hb_static);

        // ceiling verts
        for(j = 0; j < surf->numVerts; j++) {
            leaf = &doomMap.leafs[doomMap.ssLeafLookup[i] + (surf->numVerts - 1) - j];

            surf->verts[j].x = F(leaf->vertex->x);
            surf->verts[j].y = F(leaf->vertex->y);
            surf->verts[j].z = sector->ceilingheight;
        }

        surf->plane.SetNormal(surf->verts[0], surf->verts[1], surf->verts[2]);
        surf->plane.SetDistance(surf->verts[0]);
        surf->type = ST_CEILING;
        surf->typeIndex = i;

        doomMap.leafSurfaces[1][i] = surf;

        surfaces.Push(surf);
    }
}

//
// Surface_AllocateFromMap
//

void Surface_AllocateFromMap(kexWadFile &wadFile, kexDoomMap &doomMap) {
    doomMap.BuildMapFromWad(wadFile);

    doomMap.segSurfaces[0] = (surface_t**)Mem_Calloc(sizeof(surface_t*) *
        doomMap.numSegs, hb_static);
    doomMap.segSurfaces[1] = (surface_t**)Mem_Calloc(sizeof(surface_t*) *
        doomMap.numSegs, hb_static);
    doomMap.segSurfaces[2] = (surface_t**)Mem_Calloc(sizeof(surface_t*) *
        doomMap.numSegs, hb_static);

    for(int i = 0; i < doomMap.numSegs; i++) {
        Surface_AllocateFromSeg(doomMap, &doomMap.mapSegs[i]);
    }

#ifdef EXPORT_OBJ
    FILE *f = fopen("temp.obj", "w");
    fprintf(f, "o object1\n");
    int curLen = surfaces.Length();
    for(unsigned int i = 0; i < surfaces.Length(); i++) {
        for(int j = 0; j < surfaces[i]->numVerts; j++) {
            fprintf(f, "v %f %f %f\n",
                surfaces[i]->verts[j].x / 256.0f,
                surfaces[i]->verts[j].z / 256.0f,
                surfaces[i]->verts[j].y / 256.0f);
        }
    }

    int tri;

    for(unsigned int i = 0; i < surfaces.Length(); i++) {
        fprintf(f, "f %i %i %i\n", 0+(i*4)+1, 1+(i*4)+1, 2+(i*4)+1);
        fprintf(f, "f %i %i %i\n", 3+(i*4)+1, 2+(i*4)+1, 1+(i*4)+1);

        tri = 3+(i*4)+1;
    }

    tri++;
#endif

    Surface_AllocateFromLeaf(doomMap);

#ifdef EXPORT_OBJ
    fprintf(f, "o object2\n");

    for(unsigned int i = curLen; i < surfaces.Length(); i++) {
        for(int j = 0; j < surfaces[i]->numVerts; j++) {
            fprintf(f, "v %f %f %f\n",
                surfaces[i]->verts[j].x / 256.0f,
                surfaces[i]->verts[j].z / 256.0f,
                surfaces[i]->verts[j].y / 256.0f);
        }
    }

    for(unsigned int i = curLen; i < surfaces.Length(); i++) {
        fprintf(f, "f ");
        for(int j = 0; j < surfaces[i]->numVerts; j++) {
            fprintf(f, "%i ", tri++);
        }
        fprintf(f, "\n");
    }

    fclose(f);
#endif
}
