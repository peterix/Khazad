#ifndef DATATYPE__HEADER
#define DATATYPE__HEADER

#include <stdafx.h>

#include <tinyxml.h>
#include <Paths.h>

class DataBase
{

public:

    DataBase();
    virtual ~DataBase();
    bool Load(TiXmlElement* Entry, Uint32 Index);   // This must be called to index the Data Title
    virtual bool PostProcessing()              {return false;};  // Empty for overRiding in derived data classes

protected:

    string Name;
};

class ColorData: public DataBase
{

public:

    ColorData();
    ~ColorData();

    bool Load(TiXmlElement* Element, Uint32 Index);

    Uint8 getRed()      { return Red; }
    Uint8 getGreen()    { return Green; }
    Uint8 getBlue()     { return Blue; }
    // parse a value string
    void setValue (string & value);

protected:

    Uint8 Red;
    Uint8 Green;
    Uint8 Blue;
};

class TextureData: public DataBase
{

public:

    TextureData();
    ~TextureData();
    bool Load(TiXmlElement* Element, Uint32 Index);

    Path getPath() { return sPath; }

protected:

    Path sPath;
};

class MaterialData: public DataBase
{

public:

    MaterialData();
    ~MaterialData();

    bool Load(TiXmlElement* Element, Uint32 Index);
    bool PostProcessing();

    Sint16 getTextureID()               { return TextureID; }
    Sint16 getPrimaryColorID()          { return PrimaryColorID; }
    Sint16 getSecondaryColorID()        { return SecondaryColorID; }
    Sint16 getBorderColorID()           { return BorderColorID; }

    //Uint16 getHardness()                { return Hardness; }  // Use this someday for mining speed

    string getMatGloss()                { return MatGloss; }
    string getColorMode()               { return ColorMode; }
    bool getBorder()                    { return Border; }

    std::vector<Uint16> TileTypes;

protected:

    string TextureLabel;
    string PrimaryColorLabel;
    string SecondaryColorLabel;
    string BorderColorLabel;

    string MatGloss;
    string ColorMode;

    bool Border;

    Sint16 TextureID;
    Sint16 PrimaryColorID;
    Sint16 SecondaryColorID;
    Sint16 BorderColorID;

    //Uint16 Hardness;
};

class FontData: public DataBase
{

public:

    FontData();
    ~FontData();

    bool Load(TiXmlElement* Element, Uint32 Index);

    Path getPath()    { return sPath; }
    Uint16 getSize()  { return Size; }

protected:

    Path sPath;
    Uint16 Size;
};

#endif // DATATYPE__HEADER
