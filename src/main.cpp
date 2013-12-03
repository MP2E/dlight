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
// DESCRIPTION: Main application
//
//-----------------------------------------------------------------------------

#include "common.h"
#include "wad.h"
#include "mapData.h"
#include "surfaces.h"
#include "trace.h"
#include "lightmap.h"

//
// Error
//

void Error(char *error, ...) {
    va_list argptr;

    va_start(argptr,error);
    vprintf(error,argptr);
    va_end(argptr);
    printf("\n");
    exit(1);
}

//
// Va
//

char *Va(char *str, ...) {
    va_list v;
    static char vastr[1024];
	
    va_start(v, str);
    vsprintf(vastr, str,v);
    va_end(v);
    
    return vastr;	
}

//
// Main
//

int main(int argc, char **argv) {
    kexWadFile wadFile;
    kexDoomMap doomMap;
    kexLightmapBuilder builder;
    int size;
    int map = 1;
    int arg = 1;

    printf("DLight (c) 2013-2014 Samuel Villarreal\n\n");

    if(argc < 1 || argv[1] == NULL) {
        printf("Usage: dlight [options] [wadfile]\n");
        return 0;
    }

    while(1) {
        if(!strcmp(argv[arg], "-help")) {
            printf("Options:\n");
            printf("-help:              displays all known options\n");
            printf("-map:               process lightmap for MAP##\n");
            printf("-samples:           set texel sampling size (lowest = higher quaility but\n");
            printf("                    slow compile time) must be in powers of two\n");
            printf("-ambience:          set global ambience value for lightmaps (0.0 - 1.0)\n");
            printf("-size:              lightmap texture dimentions for width and height\n");
            printf("                    must be in powers of two (1, 2, 4, 8, 16, etc)\n");
            arg++;
            return 0;
        }
        else if(!strcmp(argv[arg], "-map")) {
            if(argv[arg+1] == NULL) {
                Error("Specify map number for -map\n");
                return 1;
            }

            map = atoi(argv[++arg]);
            arg++;
        }
        else if(!strcmp(argv[arg], "-samples")) {
            if(argv[arg+1] == NULL) {
                Error("Specify value for -samples\n");
                return 1;
            }

            builder.samples = atoi(argv[++arg]);
            if(builder.samples <= 0) {
                builder.samples = 1;
            }
            if(builder.samples > 128) {
                builder.samples = 128;
            }

            builder.samples = kexMath::RoundPowerOfTwo(builder.samples);
        }
        else if(!strcmp(argv[arg], "-ambience")) {
            if(argv[arg+1] == NULL) {
                Error("Specify value for -ambience\n");
                return 1;
            }

            builder.ambience = (float)atof(argv[++arg]);
            if(builder.ambience < 0) {
                builder.ambience = 0;
            }
            if(builder.ambience > 1) {
                builder.ambience = 1;
            }
        }
        else if(!strcmp(argv[arg], "-size")) {
            int lmDims;

            if(argv[arg+1] == NULL) {
                Error("Specify value for -size\n");
                return 1;
            }

            lmDims = atoi(argv[++arg]);
            if(lmDims <= 0) {
                lmDims = 1;
            }
            if(lmDims > LIGHTMAP_MAX_SIZE) {
                lmDims = LIGHTMAP_MAX_SIZE;
            }

            lmDims = kexMath::RoundPowerOfTwo(lmDims);

            builder.textureWidth = lmDims;
            builder.textureHeight = lmDims;
        }
        else {
            break;
        }
    }

    if(argv[arg] == NULL) {
        printf("Usage: dlight [options] [wadfile]\n");
        return 0;
    }

    if(!wadFile.Open(argv[arg])) {
        return 1;
    }

    printf("------------- Building level structures -------------\n\n");
    wadFile.SetCurrentMap(map);
    doomMap.BuildMapFromWad(wadFile);

    printf("------------- Allocating surfaces from level -------------\n\n");
    Surface_AllocateFromMap(doomMap);

    printf("------------- Creating lightmaps -------------\n\n");
    builder.CreateLightmaps(doomMap);
    builder.WriteTexturesToTGA();

    printf("------------- Creating lightmap lump -------------\n\n");
    byte *lm = builder.CreateLightmapLump(&size);

    printf("------------- Rebuilding wad -------------\n\n");
    wadFile.BuildNewWad(lm, size);
    wadFile.Close();

    printf("------------- Shutting down -------------\n\n");
    Mem_Purge(hb_static);
    return 0;
}
