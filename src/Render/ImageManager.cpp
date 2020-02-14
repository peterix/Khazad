#include <stdafx.h>

#include <ImageManager.h>
#include <DataManager.h>


DECLARE_SINGLETON(ImageManager)

ImageManager::ImageManager()
{

}

ImageManager::~ImageManager()
{

}

bool ImageManager::Init()
{
    // Version Check of DevIL
    if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION || iluGetInteger(ILU_VERSION_NUM) < ILU_VERSION || ilutGetInteger(ILUT_VERSION_NUM) < ILUT_VERSION)
    {
		printf ("DevIL library is out of date! Please upgrade\n");
		return false;
	}

    // Initilize all Devil components with OpenGL rendering support
    ilInit();
    iluInit();
    ilutInit();

    ilutRenderer(ILUT_OPENGL);
    ilutEnable(ILUT_OPENGL_CONV);
    ilEnable (IL_CONV_PAL);

    ilClearColour(0, 0, 0, 0);

    for(Uint32 i = 0; i < DATA->getNumTextures(); ++i)
    {
        if (DATA->getTextureData(i)->isLoneTexture())
        {
            ILuint DevilID = loadImage(DATA->getTextureData(i)->getPath(), false);
            DATA->getTextureData(i)->setDevILID(DevilID);
        }
    }

    for(Uint32 i = 0; i < DATA->getNumTextureGrids(); ++i)
    {
        TextureGridData* Grid = DATA->getTextureGridData(i);
        ILuint DevilID = loadImage(Grid->getPath(), false);

        uint16_t w = Grid->getTextureWidth();
        uint16_t h = Grid->getTextureHeight();

        std::vector<TextureData*> Textures = DATA->getTextureGridData(i)->TextureList;
        for (int j = 0; j < Textures.size(); j++)
        {
            ILuint NewID = ClipImage(DevilID, Textures[j]->getX(), Textures[j]->getY(), w, h);
            Textures[j]->setDevILID(NewID);
        }
    }

    for(Uint32 i = 0; i < DATA->getNumTextureSheets(); ++i)
    {
        TextureSheetData* Sheet = DATA->getTextureSheetData(i);
        ILuint DevilID = loadImage(Sheet->getPath(), false);

        std::vector<TextureData*> Textures = DATA->getTextureSheetData(i)->TextureList;
        for (int j = 0; j < Textures.size(); j++)
        {
            ILuint NewID = ClipImage(DevilID, Textures[j]->getX(), Textures[j]->getY(), Textures[j]->getW(), Textures[j]->getH());
            Textures[j]->setDevILID(NewID);
        }
    }

    return true;
}

ILuint ImageManager::loadImage(char* filepath, bool ColorKey)
{
    ILuint ImageID;
    ilGenImages(1, &ImageID);
    ilBindImage(ImageID);

    printf("Loading Image file: %s\n", filepath);
    if(!ilLoadImage(filepath))
    {
        cerr << "Couldn't load image " << filepath << endl;
    }

    /*
    IL_RGB
    IL_RGBA
    IL_BGR
    IL_BGRA
    IL_LUMINANCE
    IL_COLOUR_INDEX

    IL_BYTE
    IL_UNSIGNED_BYTE
    IL_SHORT
    IL_UNSIGNED_SHORT
    IL_INT
    IL_UNSIGNED_INT
    IL_FLOAT
    IL_DOUBLE
    */

    ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);

    if(ColorKey)
    {
        //convert color key
    }
    //DevilImageVector.push_back(ImageID);

    ReportDevILErrors();

    return ImageID;
}

ILuint ImageManager::ClipImage(ILuint SourceID, ILuint X, ILuint Y, ILuint W, ILuint H)
{
    ILuint ImageID;
    ilGenImages(1, &ImageID);
    ilBindImage(ImageID);
    ilTexImage(W, H, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);

    Uint8* NewImageData = ilGetData();
    ilBindImage(SourceID);
    ilCopyPixels(X, Y, 0, W, H, 1, IL_BGRA, IL_UNSIGNED_BYTE, NewImageData);

    return ImageID;
}

SDL_Surface* ImageManager::loadSurface(char* filepath, bool ColorKey)
{
	SDL_Surface* RawSurface = IMG_Load(filepath);
	SDL_Surface* ConvertedSurface = NULL;

	if(RawSurface != NULL)
	{
		if (ColorKey)
		{
			ConvertedSurface = SDL_DisplayFormat(RawSurface);
			SDL_FreeSurface(RawSurface);
			if (ConvertedSurface != NULL)
			{
				if (ColorKey)
				{
					Uint32 colorkey = SDL_MapRGB( ConvertedSurface->format, 0xFF, 0, 0xFF );
					SDL_SetColorKey( ConvertedSurface, SDL_SRCCOLORKEY, colorkey );
					return ConvertedSurface;
				}
				return ConvertedSurface;
			}
		}
		else
		{
			ConvertedSurface = SDL_DisplayFormatAlpha(RawSurface);
			if (ConvertedSurface != NULL)
			{
                SDL_FreeSurface(RawSurface);
				return ConvertedSurface;
			}
            SDL_FreeSurface(ConvertedSurface);
			return RawSurface;
		}

		return NULL;
	}
	return NULL;
}

ILuint ImageManager::GenerateMaterialImage(Sint16 MaterialID, Sint16 TextureID)
{
    MaterialData* Material = DATA->getMaterialData(MaterialID);
    TextureData* Texture = DATA->getTextureData(TextureID);

    ILuint TextureDevILID = Texture->getDevILID();

    Sint16 PrimaryColorID = Material->getPrimaryColorID();
    Sint16 SecondaryColorID = Material->getSecondaryColorID();
    Sint16 BorderColorID = Material->getBorderColorID();

    string colormode = DATA->getMaterialData(MaterialID)->getColorMode();

    if(colormode == "gradientmap")
    {
        return GenerateGradientImage(TextureDevILID, PrimaryColorID, SecondaryColorID, BorderColorID);
    }
    else if(colormode.empty() || colormode == "overlay")
    {
        return GeneratedOverLayImage(TextureDevILID, PrimaryColorID, BorderColorID);
    }
    else if(colormode == "keepimage")
    {
        return GenerateKeeperImage(TextureDevILID, BorderColorID);
    }
}

ILuint ImageManager::GenerateGradientImage(ILuint TextureDevILID, Sint16 PrimaryColorID, Sint16 SecondaryColorID, Sint16 BorderColorID)
{
    ILuint TextureImageID;
    ilGenImages(1, &TextureImageID);
    ilBindImage(TextureImageID);
    ilCopyImage(TextureDevILID);
    ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);  //Load as IL_LUMINANCE to avoid convertion?

    Uint8* TextureImageData = ilGetData();
    Uint32 width = ilGetInteger(IL_IMAGE_WIDTH);
    Uint32 height = ilGetInteger(IL_IMAGE_HEIGHT);

    ILuint MaskImageID;
    ilGenImages(1, &MaskImageID);
    ilBindImage(MaskImageID);
    ilTexImage(width, height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
    Uint8* MaskImageData = ilGetData();

    ColorData* PrimaryColor = DATA->getColorData(PrimaryColorID);
    ColorData* SecondaryColor = DATA->getColorData(SecondaryColorID);

    Uint32 bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
    if(SecondaryColor != NULL)
    {
        for(Uint32 i = 0; i < width; i++)
        {
            for(Uint32 j = 0; j < height; j++)
            {
                MaskImageData[(i * width * bpp) + (j * bpp) + 0] = SecondaryColor->getBlue();     // Blue
                MaskImageData[(i * width * bpp) + (j * bpp) + 1] = SecondaryColor->getGreen();    // Green
                MaskImageData[(i * width * bpp) + (j * bpp) + 2] = SecondaryColor->getRed();      // Red
                MaskImageData[(i * width * bpp) + (j * bpp) + 3] = 255 - TextureImageData[(i * width) + j]; // Alpha
            }
        }
    }

    ILuint NewImageID;
    ilGenImages(1, &NewImageID);
    ilBindImage(NewImageID);
    ilTexImage(width, height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
    Uint8* NewImageData = ilGetData();

    if(PrimaryColor != NULL)
    {
        for(Uint32 i = 0; i < width; i++)
        {
            for(Uint32 j = 0; j < height; j++)
            {
                NewImageData[(i * width * bpp) + (j * bpp) + 0] = PrimaryColor->getBlue(); // Blue
                NewImageData[(i * width * bpp) + (j * bpp) + 1] = PrimaryColor->getGreen(); // Green
                NewImageData[(i * width * bpp) + (j * bpp) + 2] = PrimaryColor->getRed(); // Red
                NewImageData[(i * width * bpp) + (j * bpp) + 3] = 255; // Alpha
            }
        }
    }

    ilOverlayImage(MaskImageID, 0, 0, 0);

    if (BorderColorID != -1)
    {
        ApplyBorder(NewImageID, BorderColorID);
    }

    return NewImageID;
}

ILuint ImageManager::GeneratedOverLayImage(ILuint TextureDevILID, Sint16 PrimaryColorID, Sint16 BorderColorID)
{
    ILuint TextureImageID;
    ilGenImages(1, &TextureImageID);
    ilBindImage(TextureImageID);
    ilCopyImage(TextureDevILID);
    ilConvertImage(IL_LUMINANCE_ALPHA, IL_UNSIGNED_BYTE);

    Uint8* TextureImageData = ilGetData();
    Uint32 width = ilGetInteger(IL_IMAGE_WIDTH);
    Uint32 height = ilGetInteger(IL_IMAGE_HEIGHT);

    ColorData* PrimaryColor = DATA->getColorData(PrimaryColorID);

    ILuint NewImageID;
    ilGenImages(1, &NewImageID);
    ilBindImage(NewImageID);
    ilTexImage(width, height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
    Uint8* NewImageData = ilGetData();

    Uint32 bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

    if(PrimaryColor != NULL)
    {
        for(Uint32 i = 0; i < width; i++)
        {
            for(Uint32 j = 0; j < height; j++)
            {
                float Base  = TextureImageData[(i * width * 2) + (j * 2) + 0];
                Uint8 Alpha =  TextureImageData[(i * width * 2) + (j * 2) + 1];
                Base /= 255.0;

                float OriginalBlue = PrimaryColor->getBlue();
                OriginalBlue /= 255.0;

                float OriginalGreen = PrimaryColor->getGreen();
                OriginalGreen /= 255.0;

                float OriginalRed = PrimaryColor->getRed();
                OriginalRed /= 255.0;

                // coloring using overlay mode
                if(Base >= 0.5)
                {
                    NewImageData[(i * width * bpp) + (j * bpp) + 0] = (1.0 - 2.0 * (1.0 - OriginalBlue) * (1.0 - Base)) * 255; // Blue
                    NewImageData[(i * width * bpp) + (j * bpp) + 1] = (1.0 - 2.0 * (1.0 - OriginalGreen) * (1.0 - Base)) * 255; // Green
                    NewImageData[(i * width * bpp) + (j * bpp) + 2] = (1.0 - 2.0 * (1.0 - OriginalRed) * (1.0 - Base)) * 255; // Red
                    NewImageData[(i * width * bpp) + (j * bpp) + 3] = Alpha;
                }
                else
                {
                    NewImageData[(i * width * bpp) + (j * bpp) + 0] = (2.0 * OriginalBlue * Base) * 255; // Blue
                    NewImageData[(i * width * bpp) + (j * bpp) + 1] = (2.0 * OriginalGreen * Base) * 255; // Green
                    NewImageData[(i * width * bpp) + (j * bpp) + 2] = (2.0 * OriginalRed * Base) * 255; // Red
                    NewImageData[(i * width * bpp) + (j * bpp) + 3] = Alpha;
                }
            }
        }
    }

    if (BorderColorID != -1)
    {
        ApplyBorder(NewImageID, BorderColorID);
    }

    return NewImageID;
}

ILuint ImageManager::GenerateKeeperImage(ILuint TextureDevILID, Sint16 BorderColorID)
{
    ILuint NewImageID;
    if (BorderColorID != -1)
    {
        ilGenImages(1, &NewImageID);
        ilBindImage(NewImageID);

        ilCopyImage(TextureDevILID);

        ApplyBorder(NewImageID, BorderColorID);
    }
    else
    {
        // no reason to copy large amounts of data without changes
        NewImageID = TextureDevILID;
    }
    return NewImageID;
}

void ImageManager::ApplyBorder(ILuint DevilImageID, Sint32 BorderColorID)
{
    ilBindImage(DevilImageID);
    Uint8* ImageData = ilGetData();

    Uint32 bpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
    Uint32 width = ilGetInteger(IL_IMAGE_WIDTH);
    Uint32 height = ilGetInteger(IL_IMAGE_HEIGHT);

    Uint8 Red, Green, Blue;
    ColorData* BorderColor = DATA->getColorData(BorderColorID);

    if(BorderColor != NULL)
    {
        Red = BorderColor->getRed();
        Green = BorderColor->getGreen();
        Blue = BorderColor->getBlue();

        if(ImageData != NULL)
        {
            for(Uint32 i = 0; i < width; i++)
            {
                ImageData[(i * width * bpp) +  0] = Blue;   // Blue
                ImageData[(i * width * bpp) +  1] = Green;  // Green
                ImageData[(i * width * bpp) +  2] = Red;    // Red

                ImageData[(i * width * bpp) +  3] = 255;    // Alpha

                ImageData[(i * width * bpp) + ((height - 1) * bpp) + 0] = Blue;     // Blue
                ImageData[(i * width * bpp) + ((height - 1) * bpp) + 1] = Green;    // Green
                ImageData[(i * width * bpp) + ((height - 1) * bpp) + 2] = Red;      // Red

                ImageData[(i * width * bpp) + ((height - 1) * bpp) + 3] = 255;      // Alpha
            }

            for(Uint32 j = 0; j < height; j++)
            {
                ImageData[((width - 1) * height * bpp) + (j * bpp) + 0] = Blue;     // Blue
                ImageData[((width - 1) * height * bpp) + (j * bpp) + 1] = Green;    // Green
                ImageData[((width - 1) * height * bpp) + (j * bpp) + 2] = Red;      // Red

                ImageData[((width - 1) * height * bpp) + (j * bpp) + 3] = 255;      // Alpha

                ImageData[(j * bpp) + 0] = Blue;    // Blue
                ImageData[(j * bpp) + 1] = Green;   // Green
                ImageData[(j * bpp) + 2] = Red;     // Red

                ImageData[(j * bpp) + 3] = 255;     // Alpha
            }
        }
    }
}

void ImageManager::ReportDevILErrors()
{
    ILenum Error;
    while ((Error = ilGetError()) != IL_NO_ERROR)
    {
        printf("DevIL Error %d: %s\n", Error, iluErrorString(Error));
    }
}
