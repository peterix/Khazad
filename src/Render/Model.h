/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef MODEL_HEADER
#define MODEL_HEADER

struct RenderObject;
class Model
{
    friend class ModelManager;
    public:
        inline void Render()
        {
            if(submodels[0].first == NULL)
            {
                submodels[0].first = RENDERER->CreateRenderObject( submodels[0].second );
            }
            RENDERER->CallRenderObject(submodels[0].first);
        };

        inline void Render( string submodel )
        {
            int idx = getSubmodelIndex(submodel);
            if(submodels[idx].first == NULL)
            {
                submodels[idx].first = RENDERER->CreateRenderObject( submodels[idx].second );
            }
            RENDERER->CallRenderObject(submodels[idx].first);
        };

        inline void Render( uint32_t submodel )
        {
            if(submodels[submodel].first == NULL)
            {
                submodels[submodel].first = RENDERER->CreateRenderObject( submodels[submodel].second );
            }
            RENDERER->CallRenderObject(submodels[submodel].first);
        };

        inline vector <vertex> * getSubmodelVertices ( uint32_t submodel )
        {
            return submodels[submodel].second;
        }

        inline vector <vertex> * getVertices ()
        {
            return submodels[0].second;
        }

        inline int32_t getSubmodelIndex (string submodel)
        {
            if(submodel_names.count(submodel))
            {
                return submodel_names[submodel];
            }
            return -1;
        }

    private:

        Model(map <string, pair <RenderObject *, vector <vertex> *> > & _submodels)
        {
            uint32_t i = 0;
            for(map <string, pair <RenderObject *, vector <vertex> *> >::iterator it = _submodels.begin(); it != _submodels.end(); ++it)
            {
                submodel_names[it->first] = i;
                i++;
                submodels.push_back(it->second);
            }
        }

        ~Model()
        {
            for(int i = 0; i < submodels.size(); i++ )
            {
                if(submodels[i].first != NULL)
                {
                    RENDERER->DeleteRenderObject(submodels[i].first);
                }
                delete submodels[i].second;
            }
        }

        map <string, uint32_t> submodel_names;
        vector < pair <RenderObject *, vector <vertex> *> > submodels;
};

#endif // MODEL_HEADER
