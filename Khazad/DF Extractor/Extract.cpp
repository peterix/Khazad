// Extractor
#include <Extract.h>

// zlib helper functions for de/compressing files
#include <ZlibHelper.h>

DECLARE_SINGLETON(Extractor)

Extractor::Extractor()
{
    MapLoaded = false;
}

bool Extractor::Init()
{
    // Load the Know Memory offset data
    readMemoryFile("Assets//Maps//memory.ini");

    return true;
}

Extractor::~Extractor()
{
    FreeMap();
}


bool Extractor::dumpMemory()
{
    HWND DFProcess;
    HANDLE DFHandle;
    DWORD DFProcessID;

    MapLoaded = false;

	char process_name[] = "Dwarf Fortress";
    int buffer, map_loc;
    unsigned blocks_read = 0U;

    int temp_loc, temp_locx, temp_locy, temp_locz;

    int*** Blocks = NULL;

    // Attempt to Find Process
	DFProcess = FindWindow(NULL,process_name);
	if(!DFProcess)
	{
	    printf("Cannot find window with name \"%s\"\n", process_name);
		return 3;
    }

    // And Create Handle
	GetWindowThreadProcessId(DFProcess, &DFProcessID);
	printf("Window Thread Process ID [%u] \n", (unsigned int)DFProcessID);
	DFHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, DFProcessID);

	if(!DFHandle)
	{
	    printf("Could not get application handle to hook. \n");
		return 2;
    }


    // See if this is a known Version of DF and if so get proper Memory Offsets
    if(!setMemoryOffsets(DFHandle))
    {
        printf("No PE timestamp match found, Memory Dump Aborted\n");
        return false;
    }


    // Read Map Data Blocks
    ReadProcessMemory(DFHandle, (int*)(map_offset), &map_loc, sizeof(int), NULL);
    printf("map data : 0x%.8X\n", map_loc);

    if (!map_loc)
    {
        printf("Could not find DF map information in memory...\n");
        return 1;
    }
    else
    {
        // get block count
        // df_map.x_block_count count
        ReadProcessMemory(DFHandle, (int*)(x_count_offset), &df_map.x_block_count, sizeof(int), NULL);
        printf("x_block_count: %u (%d)\n", BLOCK_SIZE * df_map.x_block_count, df_map.x_block_count);

        // df_map.y_block_count count
        ReadProcessMemory(DFHandle, (int*)(y_count_offset), &df_map.y_block_count, sizeof(int), NULL);
        printf("y_block_count: %u (%d)\n", BLOCK_SIZE * df_map.y_block_count, df_map.y_block_count);

        // z_blocks count
        ReadProcessMemory(DFHandle, (int*)(z_count_offset), &df_map.z_block_count, sizeof(int), NULL);
        printf("z_block_count: %u\n\n", df_map.z_block_count);


        // get cell count (we can just multiply it with BLOCK_SIZE)
        df_map.x_cell_count = df_map.x_block_count * BLOCK_SIZE;
        df_map.y_cell_count = df_map.y_block_count * BLOCK_SIZE;
        df_map.z_cell_count = df_map.z_block_count;

        // init map blocks (but not all)
        df_map.block = new Block***[df_map.x_block_count];

        for (Uint32 x = 0; x < df_map.x_block_count; x++)
        {
            df_map.block[x] = NULL;

//            df_map.block[x] = new Block*[df_map.y_block_count];
//            for (Uint32 y = 0; y < df_map.y_block_count; y++)
//            {
//                df_map.block[x][y] = new Block[df_map.z_block_count];
//
//            }
        }

//        printf("Maximum possible blocks: %d\n",df_map.x_block_count*df_map.y_block_count*df_map.z_block_count);


        Blocks = new int**[df_map.x_block_count];

        for (Uint32 x = 0; x < df_map.x_block_count; x++)
        {
            Blocks[x] = new int*[df_map.y_block_count];

            for (Uint32 y = 0; y < df_map.y_block_count; y++)
            {
                Blocks[x][y] = new int[df_map.z_block_count];
            }
        }

        // Initialize all the pointers to the map blocks
        for ( int x = 0; x < df_map.x_block_count; x++ )
        {
            temp_locx = map_loc + ( 4 * x );
            ReadProcessMemory( DFHandle, (int*)(temp_locx), &temp_locy, sizeof(int), NULL);

            for ( int y = 0; y < df_map.y_block_count; y++ )
            {
                ReadProcessMemory( DFHandle, (int*)(temp_locy), &temp_locz, sizeof(int), NULL);

                for ( int z = 0; z < df_map.z_block_count; z++ )
                {
                    ReadProcessMemory( DFHandle, (int*)(temp_locz), &temp_loc, sizeof(int), NULL);
                    Blocks[x][y][z] = temp_loc;

                    // allocate only the map blocks, where there is actual data.
                    if(temp_loc)
                    {
                        allocateBlocks(x, y);
                    }

                    temp_locz += 4;
                }
                temp_locy += 4;
            }
        }

        //read the memory from the map blocks
        for(int x = 0; x < df_map.x_block_count; x++)
        {
            for(int y = 0; y < df_map.y_block_count; y++)
            {
                for(int z = 0; z < df_map.z_block_count; z++)
                {
                    if (Blocks[x][y][z])
                    {
                        df_map.block[x][y][z] = new Block;

                        ReadProcessMemory(DFHandle, (int*)(Blocks[x][y][z] + tile_type_offset), &df_map.block[x][y][z]->tile_type, sizeof(short int)*BLOCK_SIZE*BLOCK_SIZE, NULL);
                        ReadProcessMemory(DFHandle, (int*)(Blocks[x][y][z] + designation_offset), &(df_map.block[x][y][z]->designation), sizeof(int)*BLOCK_SIZE*BLOCK_SIZE, NULL);
                        ReadProcessMemory(DFHandle, (int*)(Blocks[x][y][z] + occupancy_offset), &(df_map.block[x][y][z]->occupancy), sizeof(int)*BLOCK_SIZE*BLOCK_SIZE, NULL);

                        ++blocks_read;
                    }

                }
            }
        }


        printf("Blocks read into memory: %d\n", blocks_read);

        MapLoaded = true;


#ifndef NDEBUG
//        printf("Testing map data... ");
//
//        // test my data with already working data
//        if(testMapData(df_map))
//            printf("OK\n");
//        else
//            printf("WRONG DATA\n");
#endif

    }


    // block address' not needed anymore.
    if(Blocks != NULL)
    {
        for (Uint32 x = 0; x < df_map.x_block_count; x++)
        {
            if( Blocks[x] != NULL )
            {
                for (Uint32 y = 0; y < df_map.y_block_count; y++)
                {
                    if( Blocks[x][y] != NULL )
                    {
                        delete[] Blocks[x][y];
                    }
                }

                delete[] Blocks[x];
            }
        }

        delete[] Blocks;
    }


    printf("Successfully dumped memory.\n");

    return 0;
}

bool Extractor::setMemoryOffsets(HANDLE DFHandle)
{
    int Base = 0x400000;

    // New Method
    int TempOffset;
    ReadProcessMemory(DFHandle, (int*) (Base + 60), &TempOffset, sizeof(int), NULL);

    int TimeStamp;
    ReadProcessMemory(DFHandle, (int*) (Base + TempOffset + 8), &TimeStamp, sizeof(int), NULL);

    printf("TimeStampFound [%x] at Offset [%x].\n", TimeStamp, Base + TempOffset + 8);

    for (unsigned int current_mem = 0; current_mem < meminfo.size(); current_mem++ )
    {
        pe_offset = meminfo[current_mem].pe_timestamp_offset;
        pe_timestamp = meminfo[current_mem].pe_timestamp;
        map_offset = meminfo[current_mem].map_offset;
        x_count_offset = meminfo[current_mem].x_count_offset;
        y_count_offset = meminfo[current_mem].y_count_offset;
        z_count_offset = meminfo[current_mem].z_count_offset;
        tile_type_offset = meminfo[current_mem].tile_type_offset;
        designation_offset = meminfo[current_mem].designation_offset;
        occupancy_offset = meminfo[current_mem].occupancy_offset;

        // Old Method
        int OldTimeStamp;
        ReadProcessMemory(DFHandle, (int*) pe_offset, &OldTimeStamp, sizeof(int), NULL);

        if (pe_timestamp == TimeStamp || pe_timestamp == OldTimeStamp)
        {
            printf("Match found! Using version %s.\n", meminfo[current_mem].version);
            return true;
        }
        else
        {
            printf("PE timestamps do not match version %s.\n", meminfo[current_mem].version);
        }
    }
    return false;
}

// TODO: how to know when something's NULL?
bool Extractor::writeMap(char* FilePath)
{
    if(!MapLoaded)
    {
        return false;
    }

    FILE *SaveFile;
    SaveFile = fopen(FilePath,"wb");

    if(SaveFile == NULL)
    {
        printf("Can\'t create file for write.\n");
        return false;
    }
    else
    {

        fwrite(&df_map.x_block_count, sizeof(df_map.x_block_count), 1, SaveFile);
        fwrite(&df_map.y_block_count, sizeof(df_map.y_block_count), 1, SaveFile);
        fwrite(&df_map.z_block_count, sizeof(df_map.z_block_count), 1, SaveFile);

        int x, y, z;

        for (x = 0; x < df_map.x_block_count; x++ )
        {
            for (y = 0; y < df_map.y_block_count; y++ )
            {
                for (z = 0; z < df_map.z_block_count; z++ )
                {
                    if(df_map.block[x][y][z] != NULL)
                    {
                        fwrite(&x, sizeof(int), 1, SaveFile);
                        fwrite(&y, sizeof(int), 1, SaveFile);
                        fwrite(&z, sizeof(int), 1, SaveFile);

                        fwrite(&df_map.block[x][y][z]->tile_type, sizeof(short int), BLOCK_SIZE*BLOCK_SIZE, SaveFile);
                        fwrite(&df_map.block[x][y][z]->designation, sizeof(int), BLOCK_SIZE*BLOCK_SIZE, SaveFile);
                        fwrite(&df_map.block[x][y][z]->occupancy, sizeof(int), BLOCK_SIZE*BLOCK_SIZE, SaveFile);
                    }
                }
            }
        }

        x = y = z = -1;
        fwrite(&x, sizeof(int), 1, SaveFile);
        fwrite(&y, sizeof(int), 1, SaveFile);
        fwrite(&z, sizeof(int), 1, SaveFile);

    }


    freopen (FilePath,"rb",SaveFile);
    if(SaveFile == NULL)
    {
        printf("Can\'t create file for read.\n");
        return false;
    }


    FILE *SaveCompressedFile;
    char *CompressedFilePath = new char[256];

    strcpy(CompressedFilePath, FilePath);
    strcat(CompressedFilePath, ".comp");

    SaveCompressedFile = fopen(CompressedFilePath,"wb");
    if(SaveCompressedFile == NULL)
    {
        printf("Can\'t create a compressed file for write\n");
        return false;
    }

    // compress
    printf("Compressing... ");
    int ret = def(SaveFile, SaveCompressedFile, Z_BEST_COMPRESSION);

    printf("DONE\n");

    if (ret != Z_OK)
        zerr(ret);

    fclose(SaveFile);
    fclose(SaveCompressedFile);

    remove(FilePath);
    rename(CompressedFilePath, FilePath);

    return true;
}

// TODO: how to know when something's NULL?
bool Extractor::loadMap(char* FilePath)
{

    FILE *DecompressedMapFile;
    char *DecompressedFilePath = new char[256];

    strcpy(DecompressedFilePath, FilePath);
    strcat(DecompressedFilePath, ".decomp");

    FILE *MapFile;
    MapFile = fopen(DecompressedFilePath, "wb");
    unsigned blocks_read = 0U;
    int x, y, z;

    if  (MapFile == NULL)
    {
        printf("Can\'t open a decompressed file for write.\n");
        return false;
    }
    else
    {
        if(MapLoaded)
        {
            FreeMap();
        }


        //decompress
        DecompressedMapFile = fopen(FilePath,"rb");
        if  (DecompressedMapFile == NULL)
        {
            printf("Can\'t open file for read.\n");
            return false;
        }

        // Decompress
        printf("Decompressing... ");
        int ret = inf(DecompressedMapFile, MapFile);

        printf("DONE\n");

        if (ret != Z_OK)
            zerr(ret);

        fclose(DecompressedMapFile);

        freopen(DecompressedFilePath, "rb", MapFile);
        if  (MapFile == NULL)
        {
            printf("Can't create decompressed file for read.\n");
            return false;
        }

        fread(&df_map.x_block_count, sizeof(df_map.x_block_count), 1, MapFile);
        fread(&df_map.y_block_count, sizeof(df_map.y_block_count), 1, MapFile);
        fread(&df_map.z_block_count, sizeof(df_map.z_block_count), 1, MapFile);

        printf("Read from file %s\nX block size: %d\nY block size: %d\nZ levels: %d\n", FilePath, df_map.x_block_count, df_map.y_block_count, df_map.z_block_count);

        df_map.x_cell_count = df_map.x_block_count * BLOCK_SIZE;
        df_map.y_cell_count = df_map.y_block_count * BLOCK_SIZE;
        df_map.z_cell_count = df_map.z_block_count;

        df_map.block = new Block***[df_map.x_block_count];

        for (x = 0; x < df_map.x_block_count; ++x )
        {
            df_map.block[x] = NULL;
        }

        do{

            fread(&x, sizeof(int), 1, MapFile);
            fread(&y, sizeof(int), 1, MapFile);
            fread(&z, sizeof(int), 1, MapFile);

            if(x == -1 || y == -1 || z == -1)
            {
                break;
            }

            // init if necessary
            allocateBlocks(x, y);

            df_map.block[x][y][z] = new Block;

            fread(&df_map.block[x][y][z]->tile_type, sizeof(short int), BLOCK_SIZE*BLOCK_SIZE, MapFile);
            fread(&df_map.block[x][y][z]->designation, sizeof(int), BLOCK_SIZE*BLOCK_SIZE, MapFile);
            fread(&df_map.block[x][y][z]->occupancy, sizeof(int), BLOCK_SIZE*BLOCK_SIZE, MapFile);

            ++blocks_read;

        }while(true);


        printf("Blocks read into memory: %d\n", blocks_read);

        fclose(MapFile);

        remove(DecompressedFilePath);

        MapLoaded = true;
    }

    return true;
}

bool Extractor::FreeMap()
{
    if(MapLoaded)
    {
        if(df_map.block != NULL)
        {
            for (Uint32 x = 0; x < df_map.x_block_count; x++)
            {
                if(df_map.block[x] != NULL)
                {
                    for (Uint32 y = 0; y < df_map.y_block_count; y++)
                    {
                        if(df_map.block[x][y] != NULL)
                        {
                            for (Uint32 z = 0; z < df_map.z_block_count; z++)
                            {
                                if(df_map.block[x][y][z] != NULL)
                                {
                                    delete df_map.block[x][y][z];
                                }
                            }
                            delete[] df_map.block[x][y];
                        }
                    }
                    delete[] df_map.block[x];
                }
            }
            delete[] df_map.block;
        }
    }

    MapLoaded = false;
    return true;
}

int Extractor::readMemoryFile(char* FilePath)
{
    char tempString[100];
    char tempString2[100];
    int i,j,count;
    memory_info temp_meminfo;
    FILE *infile;

    printf("Opening Memory File: %s\n", FilePath);
    if ((infile = fopen(FilePath, "r")) == NULL)
    {
        fprintf(stderr, "Cannot open %s for read\n", FilePath);
        return -1;
    }

    while (fgets(tempString, 100, infile) != NULL)
    {
        if ( tempString[0] == 'v' )
        { //start a processing loop
            for (i=1; tempString[i] != 'v'; i++);
            i++;
            for (j=0; tempString[i+j] != '\n'; j++)
                tempString2[j] = tempString[j+i];
            tempString2[j] = '\0';

            //printf("%s\n",tempString2);
            sprintf(temp_meminfo.version,tempString2);
            fgets(tempString, 100, infile);

            for ( count=0; count < 9; count++ )
            {
                for (i=1; tempString[i] != 'x'; i++);
                i++;
                for (j=0; tempString[i+j] != '\n'; j++)
                    tempString2[j] = tempString[j+i];
                tempString2[j] = '\0';

                switch (count)
                {
                    case 0: temp_meminfo.pe_timestamp = strtoul(tempString2, NULL, 16); break;
                    case 1: temp_meminfo.pe_timestamp_offset = strtoul(tempString2, NULL, 16); break;
                    case 2: temp_meminfo.map_offset = strtoul(tempString2, NULL, 16); break;
                    case 3: temp_meminfo.x_count_offset = strtoul(tempString2, NULL, 16); break;
                    case 4: temp_meminfo.y_count_offset = strtoul(tempString2, NULL, 16); break;
                    case 5: temp_meminfo.z_count_offset = strtoul(tempString2, NULL, 16); break;
                    case 6: temp_meminfo.tile_type_offset = strtoul(tempString2, NULL, 16); break;
                    case 7: temp_meminfo.designation_offset = strtoul(tempString2, NULL, 16); break;
                    case 8: temp_meminfo.occupancy_offset = strtoul(tempString2, NULL, 16); break;
                }
                fgets(tempString, 100, infile);
            }

            printf("Version Data Loaded: %s\n", temp_meminfo.version);

            meminfo.push_back(temp_meminfo);
        }
    }

    return 0;
}

int Extractor::picktexture(int in)
{
    switch ( in )
    {
        case 1: //slope down
            return 3;

        case 2: //murky pool
            return 20;

        case 19: //driftwood stack
            return 8;

        case 24: //tree
            //return 3;
            return 15;

        case 25: //up-down stair frozen liquid
        case 26: //down stair frozen liquid
        case 27: //up stair frozen liquid
            return 25;

        case 32:  //open space
            return 5;

        case 34: //shrub
            return 14;

        case 35: //chasm
            return 31;

        case 36: //up-down stair lavastone
        case 37: //down stair lavastone
        case 38: //up stair lavastone
            return 32;

        case 39: //up-down stair soil
        case 40: //down stair soil
        case 41: //up stair soil
            return 10;

        case 42: //eerie pit
            return 31;

        case 43: //stone floor detailed
            return 7;

        case 44: //lavastone floor detailed
            return 32;

        case 45: //featstone? floor detailed
            return 18;

        case 46: //minstone? floor detailed [calcite]
            return 9;

        case 47: //frozen liquid floor detailed
            return 27;

        case 49: //up-down stair grass1 [muddy?]
        case 50: //down stair grass1 [muddy?]
        case 51: //up stair grass1 [muddy?]
            return 0;

        case 52: //up-down stair grass2
        case 53: //down stair grass2
        case 54: //up stair grass2
            return 0; //16;

        case 55: //up-down stair stone
        case 56: //down stair stone
        case 57: //up stair stone
            return 1;

        case 58: //up-down stair minstone
        case 59: //down stair minstone
        case 60: //up stair minstone
            return 9;

        case 61: //up-down stair featstone
        case 62: //down stair featstone
        case 63: //up stair featstone
            return 18;

        case 65: //stone fortification
            return 22;

        case 67: //campfire
            return 3;

        case 70: //fire
            return 3;

        case 79: //stone pillar
            return 1;

        case 80: //lavastone pillar
            return 32;

        case 81: //featstone pillar
            return 18;

        case 82: //minstone pillar
            return 9;

        case 83: //frozen liquid pillar
            return 27;

        case 89: //waterfall landing
            return 20;

        case 90: //river source
            return 20;

        case 176: //stone wall worn1 (most worn)
        case 177: //stone wall worn2 (sorta worn)
        case 178: //stone wall worn3 (least worn)
        case 219: //stone wall (not worn)
            return 1;

        case 231: //sapling
            return 15;

        case 233: //ramp grass dry
            return 33;

        case 234: //ramp grass dead
            return 33;

        case 235: //ramp grass1 [muddy?]
            return 0;

        case 236: //ramp grass2
            return 0; //16;

        case 237: //ramp stone
            return 1;

        case 238: //ramp lavastone
            return 32;

        case 239: //ramp featstone
            return 18;

        case 240: //ramp minstone
            return 9;

        case 241: //ramp soil
            return 10;

        case 242: //ash1
        case 243: //ash2
        case 244: //ash3
            return 32;

        case 245: //ramp frozen liquid
            return 27;

        case 258: //frozen liquid 1
        case 259: //frozen liquid 2
        case 260: //frozen liquid 3
            return 25;

        case 261: //furrowed soil [road?]
            return 21;

        case 262: //frozen liquid 0
            return 25;

        case 264: //lava
            return 24;

        case 265: //soil wall
            return 10;

        case 269: //lavastone wall rd2
        case 270: //lavastone wall r2d
        case 271: //lavastone wall r2u
        case 272: //lavastone wall ru2
        case 273: //lavastone wall l2u
        case 274: //lavastone wall lu2
        case 275: //lavastone wall l2d
        case 276: //lavastone wall ld2
        case 277: //lavastone wall lrud
        case 278: //lavastone wall rud
        case 279: //lavastone wall lrd
        case 280: //lavastone wall lru
        case 281: //lavastone wall lud
        case 282: //lavastone wall rd
        case 283: //lavastone wall ru
        case 284: //lavastone wall lu
        case 285: //lavastone wall ld
        case 286: //lavastone wall ud
        case 287: //lavastone wall lr
            return 32;

        case 288: //featstone wall rd2
        case 289: //featstone wall r2d
        case 290: //featstone wall r2u
        case 291: //featstone wall ru2
        case 292: //featstone wall l2u
        case 293: //featstone wall lu2
        case 294: //featstone wall l2d
        case 295: //featstone wall ld2
        case 296: //featstone wall lrud
        case 297: //featstone wall rud
        case 298: //featstone wall lrd
        case 299: //featstone wall lru
        case 300: //featstone wall lud
        case 301: //featstone wall rd
        case 382: //featstone wall ru
        case 303: //featstone wall lu
        case 304: //featstone wall ld
        case 305: //featstone wall ud
        case 306: //featstone wall lr
            return 18;

        case 307: //stone wall rd2
        case 308: //stone wall r2d
        case 309: //stone wall r2u
        case 310: //stone wall ru2
        case 311: //stone wall l2u
        case 312: //stone wall lu2
        case 313: //stone wall l2d
        case 314: //stone wall ld2
        case 315: //stone wall lrud
        case 316: //stone wall rud
        case 317: //stone wall lrd
        case 318: //stone wall lru
        case 319: //stone wall lud
        case 320: //stone wall rd
        case 321: //stone wall ru
        case 322: //stone wall lu
        case 323: //stone wall ld
        case 324: //stone wall ud
        case 325: //stone wall lr
            return 1;

        case 326: //lavastone fortification
            return 32;

        case 327: //featstone fortification
            return 18;

        case 328: //lavastone wall worn1 (most worn)
        case 329: //lavastone wall worn2 (middle worn)
        case 330: //lavastone wall worn3 (least worn)
        case 331: //lavastone wall
            return 32;

        case 332: //featstone wall worn1 (most worn)
        case 333: //featstone wall worn2 (middle worn)
        case 334: //featstone wall worn3 (least worn)
        case 335: //featstone wall
            return 18;

        case 336: //stone floor 1 (raw stone)
        case 337: //stone floor 2 (raw stone)
        case 338: //stone floor 3 (raw stone)
        case 339: //stone floor 4 (raw stone)
            return 17;

        case 340: //lavastone floor 1 (raw stone)
        case 341: //lavastone floor 2 (raw stone)
        case 342: //lavastone floor 3 (raw stone)
        case 343: //lavastone floor 4 (raw stone)
            return 32;

        case 344: //featstone floor 1 (raw stone)
        case 345: //featstone floor 2 (raw stone)
        case 346: //featstone floor 3 (raw stone)
        case 347: //featstone floor 4 (raw stone)
            return 18;

        case 348: //grass floor 1 (raw)
        case 349: //grass floor 2 (raw)
        case 350: //grass floor 3 (raw)
        case 351: //grass floor 4 (raw)
            return 0;

        case 352: //soil floor 1 (raw)
        case 353: //soil floor 2 (raw)
        case 354: //soil floor 3 (raw)
        case 355: //soil floor 4 (raw)
            return 10;

        case 356: //soil floor 1 wet (raw) [red sand?]
        case 357: //soil floor 2 wet (raw) [red sand?]
        case 358: //soil floor 3 wet (raw) [red sand?]
        case 359: //soil floor 4 wet (raw) [red sand?]
            return 10;

        case 360: //frozen liquid fortification
            return 27;

        case 361: //frozen liquid wall worn1 (most worn)
        case 362: //frozen liquid wall worn2 (middle worn)
        case 363: //frozen liquid wall worn3 (least worn)
        case 364: //frozen liquid wall
            return 25;

        case 365: //river n
        case 366: //river s
        case 367: //river e
        case 368: //river w
        case 369: //river nw
        case 370: //river ne
        case 371: //river sw
        case 372: //river se
            return 19;

        case 373: //stream wall n (below)
        case 374: //stream wall s (below)
        case 375: //stream wall e (below)
        case 376: //stream wall w (below)
        case 377: //stream wall nw (below)
        case 378: //stream wall ne (below)
        case 379: //stream wall sw (below)
        case 380: //stream wall se (below)
        case 381: //stream top (above)
            return 19;

        case 387: //dry grass floor1
        case 388: //dry grass floor2
        case 389: //dry grass floor3
        case 390: //dry grass floor4
            return 33;

        case 391: //dead tree
        case 392: //dead sapling
        case 393: //dead shrub
            return 13;

        case 394: //dead grass floor1
        case 395: //dead grass floor2
        case 396: //dead grass floor3
        case 397: //dead grass floor4
            return 33;

        case 398: //grass floor1b
        case 399: //grass floor2b
        case 400: //grass floor3b
        case 401: //grass floor4b
            return 0; //16;

        case 402: //stone boulder
        case 403: //lavastone boulder
        case 404: //featstone boulder
            return 18;

        case 405: //stone pebbles 1
        case 406: //stone pebbles 2
        case 407: //stone pebbles 3
        case 408: //stone pebbles 4
            return 12;

        case 409: //lavastone pebbles 1
        case 410: //lavastone pebbles 2
        case 411: //lavastone pebbles 3
        case 412: //lavastone pebbles 4
            return 12;

        case 413: //featstone pebbles 1
        case 414: //featstone pebbles 2
        case 415: //featstone pebbles 3
        case 416: //featstone pebbles 4
            return 12;

        case 417: //minstone wall rd2
        case 418: //minstone wall r2d
        case 419: //minstone wall r2u
        case 420: //minstone wall ru2
        case 421: //minstone wall l2u
        case 422: //minstone wall lu2
        case 423: //minstone wall l2d
        case 424: //minstone wall ld2
        case 425: //minstone wall lrud
        case 426: //minstone wall rud
        case 427: //minstone wall lrd
        case 428: //minstone wall lru
        case 429: //minstone wall lud
        case 430: //minstone wall rd
        case 431: //minstone wall ru
        case 432: //minstone wall lu
        case 433: //minstone wall ld
        case 434: //minstone wall ud
        case 435: //minstone wall lr
            return 9;

        case 436: //minstone fortification
            return 21;

        case 437: //minstone wall worn1
        case 438: //minstone wall worn2
        case 439: //minstone wall worn3
        case 440: //minstone wall worn4
            return 21;

        case 441: //minstone floor 1 (cavern raw)
        case 442: //minstone floor 2 (cavern raw)
        case 443: //minstone floor 3 (cavern raw)
        case 444: //minstone floor 4 (cavern raw)
            return 9;

        case 445: //minstone boulder
            return 18;

        case 446: //minstone pebbles 1
        case 447: //minstone pebbles 2
        case 448: //minstone pebbles 3
        case 449: //minstone pebbles 4
            return 12;

        case 450: //frozen liquid wall rd2
        case 451: //frozen liquid wall r2d
        case 452: //frozen liquid wall r2u
        case 453: //frozen liquid wall ru2
        case 454: //frozen liquid wall l2u
        case 455: //frozen liquid wall lu2
        case 456: //frozen liquid wall l2d
        case 457: //frozen liquid wall ld2
        case 458: //frozen liquid wall lrud
        case 459: //frozen liquid wall rud
        case 460: //frozen liquid wall lrd
        case 461: //frozen liquid wall lru
        case 462: //frozen liquid wall lud
        case 463: //frozen liquid wall rd
        case 464: //frozen liquid wall ru
        case 465: //frozen liquid wall lu
        case 466: //frozen liquid wall ld
        case 467: //frozen liquid wall ud
        case 468: //frozen liquid wall lr
            return 25;

        case 493: //constructed floor detailed
            return 7;

        case 494: //constructed fortification
            return 7;

        case 495: //constructed pillar
            return 7;

        case 496: //constructed wall rd2
        case 497: //constructed wall r2d
        case 498: //constructed wall r2u
        case 499: //constructed wall ru2
        case 500: //constructed wall l2u
        case 501: //constructed wall lu2
        case 502: //constructed wall l2d
        case 503: //constructed wall ld2
        case 504: //constructed wall lrud
        case 505: //constructed wall rud
        case 506: //constructed wall lrd
        case 507: //constructed wall lru
        case 508: //constructed wall lud
        case 509: //constructed wall rd
        case 510: //constructed wall ru
        case 511: //constructed wall lu
        case 512: //constructed wall ld
        case 513: //constructed wall ud
        case 514: //constructed wall lr
            return 22;

        case 515: //stair up-down constructed
        case 516: //stair down constructed
        case 517: //stair up constructed
            return 4;

        case 518: //ramp constructed
            return 4;

        case -1: //not assigned memory
            return 6;

        default:  //none of the above
            printf("TILE NOT FOUND [%d]\n",in);
            return -1;
    }

    return 6;
}

int Extractor::getLiquidLevel(int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {
            return df_map.block[x][y][z]->designation[x2*BLOCK_SIZE+y2] & 7; // Extracts the first 3 bits
        }
    }

    return 0;
}

short int Extractor::getTileType(int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {
            return df_map.block[x][y][z]->tile_type[x2*BLOCK_SIZE+y2];
        }
    }

    return -1;
}

int Extractor::getDesignations(int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {
            return df_map.block[x][y][z]->designation[x2*BLOCK_SIZE+y2];
        }
    }
    return -1;
}

int Extractor::getOccupancies(int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {
            return df_map.block[x][y][z]->occupancy[x2*BLOCK_SIZE+y2];
        }
    }
    return -1;
}

bool Extractor::isOpenTerrain(int in)
{
    switch (in)
    {
        //case -1: //uninitialized tile
        case 1: //slope down
        case 19: //driftwood stack
        case 24: //tree
        case 25: //up-down stair frozen liquid
        case 26: //down stair frozen liquid
        case 27: //up stair frozen liquid
        case 32: //open space
        case 34: //shrub
        case 35: //chasm
        case 36: //up-down stair lavastone
        case 37: //down stair lavastone
        case 38: //up stair lavastone
        case 39: //up-down stair soil
        case 40: //down stair soil
        case 41: //up stair soil
        case 42: //eerie pit
        case 43: //stone floor detailed
        case 44: //lavastone floor detailed
        case 45: //featstone? floor detailed
        case 46: //minstone? floor detailed [calcite]
        case 47: //frozen liquid floor detailed
        case 49: //up-down stair grass1 [muddy?]
        case 50: //down stair grass1 [muddy?]
        case 51: //up stair grass1 [muddy?]
        case 52: //up-down stair grass2
        case 53: //down stair grass2
        case 54: //up stair grass2
        case 55: //up-down stair stone
        case 56: //down stair stone
        case 57: //up stair stone
        case 58: //up-down stair minstone
        case 59: //down stair minstone
        case 60: //up stair minstone
        case 61: //up-down stair featstone
        case 62: //down stair featstone
        case 63: //up stair featstone
        case 67: //campfire
        case 70: //fire
        /*
        case 79: //stone pillar
        case 80: //lavastone pillar
        case 81: //featstone pillar
        case 82: //minstone pillar
        case 83: //frozen liquid pillar
        */
        case 231: //sapling
        case 233: //ramp grass dry
        case 234: //ramp grass dead
        case 235: //ramp grass1 [muddy?]
        case 236: //ramp grass2
        case 237: //ramp stone
        case 238: //ramp lavastone
        case 239: //ramp featstone
        case 240: //ramp minstone
        case 241: //ramp soil
        case 242: //ash1
        case 243: //ash2
        case 244: //ash3
        case 245: //ramp frozen liquid
        case 261: //furrowed soil [road?]
        case 262: //Ice floor
        case 336: //stone floor 1 (raw stone)
        case 337: //stone floor 2 (raw stone)
        case 338: //stone floor 3 (raw stone)
        case 339: //stone floor 4 (raw stone)
        case 340: //lavastone floor 1 (raw stone)
        case 341: //lavastone floor 2 (raw stone)
        case 342: //lavastone floor 3 (raw stone)
        case 343: //lavastone floor 4 (raw stone)
        case 344: //featstone floor 1 (raw stone)
        case 345: //featstone floor 2 (raw stone)
        case 346: //featstone floor 3 (raw stone)
        case 347: //featstone floor 4 (raw stone)
        case 348: //grass floor 1 (raw)
        case 349: //grass floor 2 (raw)
        case 350: //grass floor 3 (raw)
        case 351: //grass floor 4 (raw)
        case 352: //soil floor 1 (raw)
        case 353: //soil floor 2 (raw)
        case 354: //soil floor 3 (raw)
        case 355: //soil floor 4 (raw)
        case 356: //soil floor 1 wet (raw) [red sand?]
        case 357: //soil floor 2 wet (raw) [red sand?]
        case 358: //soil floor 3 wet (raw) [red sand?]
        case 359: //soil floor 4 wet (raw) [red sand?]
        case 381: //stream top (above)
        case 387: //dry grass floor1
        case 388: //dry grass floor2
        case 389: //dry grass floor3
        case 390: //dry grass floor4
        case 391: //dead tree
        case 392: //dead sapling
        case 393: //dead shrub
        case 394: //dead grass floor1
        case 395: //dead grass floor2
        case 396: //dead grass floor3
        case 397: //dead grass floor4
        case 398: //grass floor1b
        case 399: //grass floor2b
        case 400: //grass floor3b
        case 401: //grass floor4b
        case 402: //stone boulder
        case 403: //lavastone boulder
        case 404: //featstone boulder
        case 405: //stone pebbles 1
        case 406: //stone pebbles 2
        case 407: //stone pebbles 3
        case 408: //stone pebbles 4
        case 409: //lavastone pebbles 1
        case 410: //lavastone pebbles 2
        case 411: //lavastone pebbles 3
        case 412: //lavastone pebbles 4
        case 413: //featstone pebbles 1
        case 414: //featstone pebbles 2
        case 415: //featstone pebbles 3
        case 416: //featstone pebbles 4
        case 441: //minstone floor 1 (cavern raw)
        case 442: //minstone floor 2 (cavern raw)
        case 443: //minstone floor 3 (cavern raw)
        case 444: //minstone floor 4 (cavern raw)
        case 445: //minstone boulder
        case 446: //minstone pebbles 1
        case 447: //minstone pebbles 2
        case 448: //minstone pebbles 3
        case 449: //minstone pebbles 4
        case 493: //constructed floor detailed
        //case 495: //constructed pillar
        case 515: //stair up-down constructed
        case 516: //stair down constructed
        case 517: //stair up constructed
        case 518: //ramp constructed

            return true;
            break;
    }

    return false;
}

bool Extractor::isFloorTerrain(int in)
{
    switch (in)
    {
        case 2:   //murky pool

        case 19: //driftwood stack
        case 24: //tree
       // case 27: //up stair frozen liquid
        case 34: //shrub
        case 35: //Chasm
       // case 38: //up stair lavastone
       // case 41: //up stair soil
        case 42: //eerie pit
        case 43: //stone floor detailed
        case 44: //lavastone floor detailed
        case 45: //featstone? floor detailed
        case 46: //minstone? floor detailed [calcite]
        case 47: //frozen liquid floor detailed
        /*
        case 51: //up stair grass1 [muddy?]
        case 54: //up stair grass2
        case 57: //up stair stone
        case 60: //up stair minstone
        case 63: //up stair featstone
        */
        case 67: //campfire
        case 70: //fire
        /*
        case 79: //stone pillar
        case 80: //lavastone pillar
        case 81: //featstone pillar
        case 82: //minstone pillar
        case 83: //frozen liquid pillar
        */
        case 89: //waterfall landing
        case 90: //river source

        case 231: //sapling
        /*
        case 233: //ramp grass dry
        case 234: //ramp grass dead
        case 235: //ramp grass1 [muddy?]
        case 236: //ramp grass2
        case 237: //ramp stone
        case 238: //ramp lavastone
        case 239: //ramp featstone
        case 240: //ramp minstone
        case 241: //ramp soil
        */
        case 242: //ash1
        case 243: //ash2
        case 244: //ash3
        case 245: //ramp frozen liquid
        case 261: //furrowed soil [road?]
        case 262: //Ice floor
        case 264: //Lava bottom of map
        case 336: //stone floor 1 (raw stone)
        case 337: //stone floor 2 (raw stone)
        case 338: //stone floor 3 (raw stone)
        case 339: //stone floor 4 (raw stone)
        case 340: //lavastone floor 1 (raw stone)
        case 341: //lavastone floor 2 (raw stone)
        case 342: //lavastone floor 3 (raw stone)
        case 343: //lavastone floor 4 (raw stone)
        case 344: //featstone floor 1 (raw stone)
        case 345: //featstone floor 2 (raw stone)
        case 346: //featstone floor 3 (raw stone)
        case 347: //featstone floor 4 (raw stone)
        case 348: //grass floor 1 (raw)
        case 349: //grass floor 2 (raw)
        case 350: //grass floor 3 (raw)
        case 351: //grass floor 4 (raw)
        case 352: //soil floor 1 (raw)
        case 353: //soil floor 2 (raw)
        case 354: //soil floor 3 (raw)
        case 355: //soil floor 4 (raw)
        case 356: //soil floor 1 wet (raw) [red sand?]
        case 357: //soil floor 2 wet (raw) [red sand?]
        case 358: //soil floor 3 wet (raw) [red sand?]
        case 359: //soil floor 4 wet (raw) [red sand?]

        case 365: //river n
        case 366: //river s
        case 367: //river e
        case 368: //river w
        case 369: //river nw
        case 370: //river ne
        case 371: //river sw
        case 372: //river se

        case 373: //stream wall n (below)
        case 374: //stream wall s (below)
        case 375: //stream wall e (below)
        case 376: //stream wall w (below)
        case 377: //stream wall nw (below)
        case 378: //stream wall ne (below)
        case 379: //stream wall sw (below)
        case 380: //stream wall se (below)

        case 387: //dry grass floor1
        case 388: //dry grass floor2
        case 389: //dry grass floor3
        case 390: //dry grass floor4
        case 391: //dead tree
        case 392: //dead sapling
        case 393: //dead shrub
        case 394: //dead grass floor1
        case 395: //dead grass floor2
        case 396: //dead grass floor3
        case 397: //dead grass floor4
        case 398: //grass floor1b
        case 399: //grass floor2b
        case 400: //grass floor3b
        case 401: //grass floor4b
        case 402: //stone boulder
        case 403: //lavastone boulder
        case 404: //featstone boulder
        case 405: //stone pebbles 1
        case 406: //stone pebbles 2
        case 407: //stone pebbles 3
        case 408: //stone pebbles 4
        case 409: //lavastone pebbles 1
        case 410: //lavastone pebbles 2
        case 411: //lavastone pebbles 3
        case 412: //lavastone pebbles 4
        case 413: //featstone pebbles 1
        case 414: //featstone pebbles 2
        case 415: //featstone pebbles 3
        case 416: //featstone pebbles 4
        case 441: //minstone floor 1 (cavern raw)
        case 442: //minstone floor 2 (cavern raw)
        case 443: //minstone floor 3 (cavern raw)
        case 444: //minstone floor 4 (cavern raw)
        case 445: //minstone boulder
        case 446: //minstone pebbles 1
        case 447: //minstone pebbles 2
        case 448: //minstone pebbles 3
        case 449: //minstone pebbles 4
        case 493: //constructed floor detailed
        //case 495: //constructed pillar
        case 517: //stair up constructed
        //case 518: //ramp constructed
            return true;
            break;
    }

    return false;
}

bool Extractor::isRampTerrain(int in)
{
    switch (in)
    {
        case 233: //ramp grass dry
        case 234: //ramp grass dead
        case 235: //ramp grass1 [muddy?]
        case 236: //ramp grass2
        case 237: //ramp stone
        case 238: //ramp lavastone
        case 239: //ramp featstone
        case 240: //ramp minstone
        case 241: //ramp soil
        case 245: //ramp frozen liquid
        case 518: //ramp constructed
            return true;
            break;
    }

    return false;
}

bool Extractor::isStairTerrain(int in)
{
    switch (in)
    {
        case 25: //up-down stair frozen liquid
        case 26: //down stair frozen liquid
        case 27: //up stair frozen liquid
        case 36: //up-down stair lavastone
        case 37: //down stair lavastone
        case 38: //up stair lavastone
        case 39: //up-down stair soil
        case 40: //down stair soil
        case 41: //up stair soil
        case 49: //up-down stair grass1 [muddy?]
        case 50: //down stair grass1 [muddy?]
        case 51: //up stair grass1 [muddy?]
        case 52: //up-down stair grass2
        case 53: //down stair grass2
        case 54: //up stair grass2
        case 55: //up-down stair stone
        case 56: //down stair stone
        case 57: //up stair stone
        case 58: //up-down stair minstone
        case 59: //down stair minstone
        case 60: //up stair minstone
        case 61: //up-down stair featstone
        case 62: //down stair featstone
        case 63: //up stair featstone
        case 515: //stair up-down constructed
        case 516: //stair down constructed
        case 517: //stair up constructed
            return true;
            break;
    }

    return false;
}

bool Extractor::isWallTerrain(int in)
{
    switch (in)
    {
        case 65: //stone fortification
        case 79: //stone pillar
        case 80: //lavastone pillar
        case 81: //featstone pillar
        case 82: //minstone pillar
        case 83: //frozen liquid pillar
        case 176: //stone wall worn1 (most worn)
        case 177: //stone wall worn2 (sorta worn)
        case 178: //stone wall worn3 (least worn)
        case 219: //stone wall (not worn)
        case 258: //frozen liquid 1
        case 259: //frozen liquid 2
        case 260: //frozen liquid 3
        case 262: //frozen liquid 0
        case 265: //soil wall
        case 269: //lavastone wall rd2
        case 270: //lavastone wall r2d
        case 271: //lavastone wall r2u
        case 272: //lavastone wall ru2
        case 273: //lavastone wall l2u
        case 274: //lavastone wall lu2
        case 275: //lavastone wall l2d
        case 276: //lavastone wall ld2
        case 277: //lavastone wall lrud
        case 278: //lavastone wall rud
        case 279: //lavastone wall lrd
        case 280: //lavastone wall lru
        case 281: //lavastone wall lud
        case 282: //lavastone wall rd
        case 283: //lavastone wall ru
        case 284: //lavastone wall lu
        case 285: //lavastone wall ld
        case 286: //lavastone wall ud
        case 287: //lavastone wall lr
        case 288: //featstone wall rd2
        case 289: //featstone wall r2d
        case 290: //featstone wall r2u
        case 291: //featstone wall ru2
        case 292: //featstone wall l2u
        case 293: //featstone wall lu2
        case 294: //featstone wall l2d
        case 295: //featstone wall ld2
        case 296: //featstone wall lrud
        case 297: //featstone wall rud
        case 298: //featstone wall lrd
        case 299: //featstone wall lru
        case 300: //featstone wall lud
        case 301: //featstone wall rd
        case 382: //featstone wall ru
        case 303: //featstone wall lu
        case 304: //featstone wall ld
        case 305: //featstone wall ud
        case 306: //featstone wall lr
        case 307: //stone wall rd2
        case 308: //stone wall r2d
        case 309: //stone wall r2u
        case 310: //stone wall ru2
        case 311: //stone wall l2u
        case 312: //stone wall lu2
        case 313: //stone wall l2d
        case 314: //stone wall ld2
        case 315: //stone wall lrud
        case 316: //stone wall rud
        case 317: //stone wall lrd
        case 318: //stone wall lru
        case 319: //stone wall lud
        case 320: //stone wall rd
        case 321: //stone wall ru
        case 322: //stone wall lu
        case 323: //stone wall ld
        case 324: //stone wall ud
        case 325: //stone wall lr
        case 326: //lavastone fortification
        case 327: //featstone fortification
        case 328: //lavastone wall worn1 (most worn)
        case 329: //lavastone wall worn2 (middle worn)
        case 330: //lavastone wall worn3 (least worn)
        case 331: //lavastone wall
        case 332: //featstone wall worn1 (most worn)
        case 333: //featstone wall worn2 (middle worn)
        case 334: //featstone wall worn3 (least worn)
        case 335: //featstone wall
        case 360: //frozen liquid fortification
        case 361: //frozen liquid wall worn1 (most worn)
        case 362: //frozen liquid wall worn2 (middle worn)
        case 363: //frozen liquid wall worn3 (least worn)
        case 364: //frozen liquid wall
        case 417: //minstone wall rd2
        case 418: //minstone wall r2d
        case 419: //minstone wall r2u
        case 420: //minstone wall ru2
        case 421: //minstone wall l2u
        case 422: //minstone wall lu2
        case 423: //minstone wall l2d
        case 424: //minstone wall ld2
        case 425: //minstone wall lrud
        case 426: //minstone wall rud
        case 427: //minstone wall lrd
        case 428: //minstone wall lru
        case 429: //minstone wall lud
        case 430: //minstone wall rd
        case 431: //minstone wall ru
        case 432: //minstone wall lu
        case 433: //minstone wall ld
        case 434: //minstone wall ud
        case 435: //minstone wall lr
        case 436: //minstone fortification
        case 437: //minstone wall worn1
        case 438: //minstone wall worn2
        case 439: //minstone wall worn3
        case 440: //minstone wall worn4
        case 450: //frozen liquid wall rd2
        case 451: //frozen liquid wall r2d
        case 452: //frozen liquid wall r2u
        case 453: //frozen liquid wall ru2
        case 454: //frozen liquid wall l2u
        case 455: //frozen liquid wall lu2
        case 456: //frozen liquid wall l2d
        case 457: //frozen liquid wall ld2
        case 458: //frozen liquid wall lrud
        case 459: //frozen liquid wall rud
        case 460: //frozen liquid wall lrd
        case 461: //frozen liquid wall lru
        case 462: //frozen liquid wall lud
        case 463: //frozen liquid wall rd
        case 464: //frozen liquid wall ru
        case 465: //frozen liquid wall lu
        case 466: //frozen liquid wall ld
        case 467: //frozen liquid wall ud
        case 468: //frozen liquid wall lr
        case 494: //constructed fortification
        case 495: //constructed pillar
        case 496: //constructed wall rd2
        case 497: //constructed wall r2d
        case 498: //constructed wall r2u
        case 499: //constructed wall ru2
        case 500: //constructed wall l2u
        case 501: //constructed wall lu2
        case 502: //constructed wall l2d
        case 503: //constructed wall ld2
        case 504: //constructed wall lrud
        case 505: //constructed wall rud
        case 506: //constructed wall lrd
        case 507: //constructed wall lru
        case 508: //constructed wall lud
        case 509: //constructed wall rd
        case 510: //constructed wall ru
        case 511: //constructed wall lu
        case 512: //constructed wall ld
        case 513: //constructed wall ud
        case 514: //constructed wall lr
            return true;
            break;
    }

    return false;
}

bool Extractor::isDesignationFlag(unsigned int flag, int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {
            if(flag < 32)
            {
                return (df_map.block[x][y][z]->designation[x2*BLOCK_SIZE+y2] & (1 << flag));
            }
        }
    }
    return false;
}

bool Extractor::isOcupancyFlag(unsigned int flag, int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {
            if(flag < 32)
            {
                return (df_map.block[x][y][z]->occupancy[x2*BLOCK_SIZE+y2] & (1 << flag));
            }
        }
    }
    return false;
}

// NOTE: UNTESTED!
int Extractor::DesignationBitBlock(unsigned int Start, unsigned int Size, int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {

            if(Start < 32)
            {
                int Total = 0;
                for(int i = 0; i < Size; ++i)
                {
                    Total += ((df_map.block[x][y][z]->designation[x2*BLOCK_SIZE+y2] & (1 << Start + i)) >> Start);
                }
            }
        }
    }
    return -1;
}

// NOTE: UNTESTED!
int Extractor::OccupancyBitBlock(unsigned int Start, unsigned int Size, int x, int y, int z)
{
    if(x < df_map.x_cell_count && x >= 0 && y < df_map.y_cell_count && y >= 0)
    {
        int x2, y2;
        convertToDfMapCoords(x, y, x, y, x2, y2);

        if(z < df_map.z_block_count && z >= 0 && df_map.block[x][y][z] != NULL)
        {

            if(Start < 32)
            {
                int Total = 0;
                for(int i = 0; i < Size; ++i)
                {
                    Total += ((df_map.block[x][y][z]->occupancy[x2*BLOCK_SIZE+y2] & (1 << Start + i)) >> Start);
                }
            }
        }
    }
    return -1;
}

// NOTE: use this function to test for validity of new map data structures
bool Extractor::testMapData(DfMap df_map)
{
//
//// conversion:
////    df_map.block[x][y][z]->tile_type[x2*BLOCK_SIZE+y2]
////    Tiles[x * BLOCK_SIZE + x2][y * BLOCK_SIZE + y2][z]
//
//    bool ret = true;
//
//
//
//    // test every value in map data structures
//    for (int x = 0; x < df_map.x_block_count; ++x)
//    {
//        if(df_map.block[x] != NULL)
//        {
//            for (int y = 0; y < df_map.y_block_count; ++y)
//            {
//                if(df_map.block[x][y] != NULL)
//                {
//                    for (int z = 0; z < df_map.z_block_count; ++z)
//                    {
//                        for (int x2 = 0; x2 < BLOCK_SIZE; ++x2)
//                        {
//                            for (int y2 = 0; y2< BLOCK_SIZE; ++y2)
//                            {
//                                // testing for false-positive (if you uncomment this line, function should return false)
////                                df_map.block[x][y][z]->tile_type[(x2*BLOCK_SIZE)+y2] = df_map.block[x][y][z]->tile_type[(x2*BLOCK_SIZE)+y2]+1;
//
//                                ret &= df_map.block[x][y][z]->tile_type[(x2*BLOCK_SIZE)+y2] == Tiles[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z];
//                                ret &= df_map.block[x][y][z]->designation[(x2*BLOCK_SIZE)+y2] == Designations[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z];
//                                ret &= df_map.block[x][y][z]->occupancy[(x2*BLOCK_SIZE)+y2] == Ocupancy[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z];
//
//                                // here I also test the conversion function
//                                int xx = (x*BLOCK_SIZE)+x2;
//                                int yy = (y*BLOCK_SIZE)+y2;
//
//                                int xx2, yy2;
//                                convertToDfMapCoords(xx, yy, xx, yy, xx2, yy2);
//
//                                ret &= df_map.block[xx][yy][z]->tile_type[(xx2*BLOCK_SIZE)+yy2] == Tiles[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z];
//                                ret &= df_map.block[xx][yy][z]->designation[(xx2*BLOCK_SIZE)+yy2] == Designations[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z];
//                                ret &= df_map.block[xx][yy][z]->occupancy[(xx2*BLOCK_SIZE)+yy2] == Ocupancy[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z];
//
//                                if(!ret)
//                                {
//
//                                    //printf("%d %d\n", df_map.block[x][y][z]->designation[(x2*BLOCK_SIZE)+y2],Designations[(x*BLOCK_SIZE)+x2][(y*BLOCK_SIZE)+y2][z]);
//                                    printf("%d %d %d %d\n", x,y,x2,y2);
//                                    printf("%d %d %d %d\n", xx,yy,xx2,yy2);
//
//                                    return ret;
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//    }

    //return ret;

    return true;
}

// converts the (x,y,z) cell coords to internal coords
// out_y, out_x - block coords
// out_y2, out_x2 - cell coords in that block
void Extractor::convertToDfMapCoords(int x, int y, int &out_x, int &out_y, int &out_x2, int &out_y2)
{
    out_x2 = x % BLOCK_SIZE;
    out_y2 = y % BLOCK_SIZE;
    out_y = int(y / BLOCK_SIZE);
    out_x = int(x / BLOCK_SIZE);
}

void Extractor::allocateBlocks(int x, int y)
{
    if( df_map.block == NULL )
    {
        printf("Unexpected error - DfMap.block is not initialized!\n");
        return;
    }

    if( df_map.block[x] == NULL )
    {
        df_map.block[x] = new Block**[df_map.y_block_count];

        for (Uint32 y2 = 0; y2 < df_map.y_block_count; y2++)
        {
            df_map.block[x][y2] = NULL;
        }
    }

    if( df_map.block[x][y] == NULL )
    {
        df_map.block[x][y] = new Block*[df_map.z_block_count];

        for (Uint32 z2 = 0; z2 < df_map.z_block_count; z2++)
        {
            df_map.block[x][y][z2] = NULL;
        }
    }
}
