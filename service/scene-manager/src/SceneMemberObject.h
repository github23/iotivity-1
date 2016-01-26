//******************************************************************
//
// Copyright 2016 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SM_SCENEMEMBEROBJECT_H_
#define SM_SCENEMEMBEROBJECT_H_

#include <cstdbool>
#include <list>
#include <functional>
#include <string>
#include <map>

#include "octypes.h"

#include "RCSRemoteResourceObject.h"
#include "RCSResourceObject.h"

namespace OIC
{
    namespace Service
    {
        class SceneMemberObject
        {
        public:
            typedef std::shared_ptr< SceneMemberObject > Ptr;

        public:
            SceneMemberObject(RCSRemoteResourceObject::Ptr);

        public:
            void setName(std::string);
            std::string getName();

            RCSResourceAttributes getSceneMappingItem(std::string);
            std::map< std::string, RCSResourceAttributes > getSceneMapping();
            bool hasSceneMappingitem(std::string);
            RCSRemoteResourceObject::Ptr getRemoteResourceObject();

        private:
            unsigned int Id;
            std::string name;
            std::string link;
            std::map< std::string, RCSResourceAttributes > sceneMapping;

            RCSRemoteResourceObject::Ptr RemoteObjectPtr;
            RCSResourceObject::Ptr sceneMemberResourcePtr;
        };
    } /* namespace Service */
} /* namespace OIC */
#endif /* SM_SCENEMEMBEROBJECT_H_ */
