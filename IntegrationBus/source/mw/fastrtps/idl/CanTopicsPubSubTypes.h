// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*! 
 * @file CanTopicsPubSubTypes.h
 * This header file contains the declaration of the serialization functions.
 *
 * This file was generated by the tool fastcdrgen.
 */


#ifndef _IB_SIM_CAN_IDL_CANTOPICS_PUBSUBTYPES_H_
#define _IB_SIM_CAN_IDL_CANTOPICS_PUBSUBTYPES_H_

#include <fastrtps/TopicDataType.h>

#include "CanTopics.h"

namespace ib
{
    namespace sim
    {
        namespace can
        {
            namespace idl
            {
                /*!
                 * @brief This class represents the TopicDataType of the type CanMessageFlags defined by the user in the IDL file.
                 * @ingroup CANTOPICS
                 */
                class CanMessageFlagsPubSubType : public eprosima::fastrtps::TopicDataType {
                public:
                        typedef CanMessageFlags type;

                	CanMessageFlagsPubSubType();
                	virtual ~CanMessageFlagsPubSubType();
                	bool serialize(void *data, eprosima::fastrtps::rtps::SerializedPayload_t *payload);
                	bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload, void *data);
                        std::function<uint32_t()> getSerializedSizeProvider(void* data);
                	bool getKey(void *data, eprosima::fastrtps::rtps::InstanceHandle_t *ihandle);
                	void* createData();
                	void deleteData(void * data);
                	MD5 m_md5;
                	unsigned char* m_keyBuffer;
                };
                typedef uint32_t transmitIdT;
                /*!
                 * @brief This class represents the TopicDataType of the type CanMessage defined by the user in the IDL file.
                 * @ingroup CANTOPICS
                 */
                class CanMessagePubSubType : public eprosima::fastrtps::TopicDataType {
                public:
                        typedef CanMessage type;

                	CanMessagePubSubType();
                	virtual ~CanMessagePubSubType();
                	bool serialize(void *data, eprosima::fastrtps::rtps::SerializedPayload_t *payload);
                	bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload, void *data);
                        std::function<uint32_t()> getSerializedSizeProvider(void* data);
                	bool getKey(void *data, eprosima::fastrtps::rtps::InstanceHandle_t *ihandle);
                	void* createData();
                	void deleteData(void * data);
                	MD5 m_md5;
                	unsigned char* m_keyBuffer;
                };
                /*!
                 * @brief This class represents the TopicDataType of the type CanControllerStatus defined by the user in the IDL file.
                 * @ingroup CANTOPICS
                 */
                class CanControllerStatusPubSubType : public eprosima::fastrtps::TopicDataType {
                public:
                        typedef CanControllerStatus type;

                	CanControllerStatusPubSubType();
                	virtual ~CanControllerStatusPubSubType();
                	bool serialize(void *data, eprosima::fastrtps::rtps::SerializedPayload_t *payload);
                	bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload, void *data);
                        std::function<uint32_t()> getSerializedSizeProvider(void* data);
                	bool getKey(void *data, eprosima::fastrtps::rtps::InstanceHandle_t *ihandle);
                	void* createData();
                	void deleteData(void * data);
                	MD5 m_md5;
                	unsigned char* m_keyBuffer;
                };
                /*!
                 * @brief This class represents the TopicDataType of the type CanTransmitAcknowledge defined by the user in the IDL file.
                 * @ingroup CANTOPICS
                 */
                class CanTransmitAcknowledgePubSubType : public eprosima::fastrtps::TopicDataType {
                public:
                        typedef CanTransmitAcknowledge type;

                	CanTransmitAcknowledgePubSubType();
                	virtual ~CanTransmitAcknowledgePubSubType();
                	bool serialize(void *data, eprosima::fastrtps::rtps::SerializedPayload_t *payload);
                	bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload, void *data);
                        std::function<uint32_t()> getSerializedSizeProvider(void* data);
                	bool getKey(void *data, eprosima::fastrtps::rtps::InstanceHandle_t *ihandle);
                	void* createData();
                	void deleteData(void * data);
                	MD5 m_md5;
                	unsigned char* m_keyBuffer;
                };
                /*!
                 * @brief This class represents the TopicDataType of the type CanConfigureBaudrate defined by the user in the IDL file.
                 * @ingroup CANTOPICS
                 */
                class CanConfigureBaudratePubSubType : public eprosima::fastrtps::TopicDataType {
                public:
                        typedef CanConfigureBaudrate type;

                	CanConfigureBaudratePubSubType();
                	virtual ~CanConfigureBaudratePubSubType();
                	bool serialize(void *data, eprosima::fastrtps::rtps::SerializedPayload_t *payload);
                	bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload, void *data);
                        std::function<uint32_t()> getSerializedSizeProvider(void* data);
                	bool getKey(void *data, eprosima::fastrtps::rtps::InstanceHandle_t *ihandle);
                	void* createData();
                	void deleteData(void * data);
                	MD5 m_md5;
                	unsigned char* m_keyBuffer;
                };
                /*!
                 * @brief This class represents the TopicDataType of the type CanSetControllerMode defined by the user in the IDL file.
                 * @ingroup CANTOPICS
                 */
                class CanSetControllerModePubSubType : public eprosima::fastrtps::TopicDataType {
                public:
                        typedef CanSetControllerMode type;

                	CanSetControllerModePubSubType();
                	virtual ~CanSetControllerModePubSubType();
                	bool serialize(void *data, eprosima::fastrtps::rtps::SerializedPayload_t *payload);
                	bool deserialize(eprosima::fastrtps::rtps::SerializedPayload_t *payload, void *data);
                        std::function<uint32_t()> getSerializedSizeProvider(void* data);
                	bool getKey(void *data, eprosima::fastrtps::rtps::InstanceHandle_t *ihandle);
                	void* createData();
                	void deleteData(void * data);
                	MD5 m_md5;
                	unsigned char* m_keyBuffer;
                };
            }
        }
    }
}

#endif // _IB_SIM_CAN_IDL_CANTOPICS_PUBSUBTYPES_H_