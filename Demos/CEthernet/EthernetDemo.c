/* Copyright (c) Vector Informatik GmbH. All rights reserved. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "ib/capi/IntegrationBus.h"

#ifdef WIN32
#include "Windows.h"
#   define SleepMs(X) Sleep(X)
#else
#   include "unistd.h"
#define SleepMs(X) usleep((X)*1000)
#endif

char* LoadFile(char const* path)
{
    size_t length = 0;
    char* result = NULL;
    FILE* f = fopen(path, "rb");

    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* buffer = (char*)malloc((length + 1) * sizeof(char));
        if (buffer)
        {
            size_t num = fread(buffer, sizeof(uint8_t), length, f);
            if(num != length)
            {
                printf("Warning: short read on config file: %zu/%zu",
                    num, length);
                exit(1);
            }
            buffer[length] = '\0';
            result = buffer;
        }
        fclose(f);
    }
    return result;
}

void MacToBytes(uint8_t* outBytes, const char* mac)
{
    char macCopy[18];
    memcpy(macCopy, mac, sizeof(macCopy)-1);

    char* ptrMac = macCopy;
    uint8_t* ptrBytes = outBytes;
    for (int i = 0; i < 18; i++) {
        if (macCopy[i] == ':' || i==17)
        {
            macCopy[i] = '\0';
            long curByte = strtol(ptrMac, NULL, 16);
            *ptrBytes = (uint8_t)strtol(ptrMac, NULL, 16);
            ptrMac += 3;
            ptrBytes += 1;
        }
    }
}

ib_SimulationParticipant* participant;
ib_EthernetController* ethernetController1;
ib_EthernetController* ethernetController2;

char* participantName;
uint8_t ethernetMessageCounter = 0;
#define SOURCE_MAC_SIZE 6
#define DESTINATION_MAC_SIZE 6
#define ETHERTYPE_MAC_SIZE 2
#define PAYLOAD_OFFSET SOURCE_MAC_SIZE + DESTINATION_MAC_SIZE + ETHERTYPE_MAC_SIZE

typedef struct  {
    uint32_t someInt;
} TransmitContext;

TransmitContext transmitContext;

void AckCallback(void* context, ib_EthernetController* controller, struct ib_EthernetTransmitAcknowledge* cAck)
{
    TransmitContext* tc = (TransmitContext*) cAck->userContext;
    printf(">> %i for Ethernet Message with transmitId=%i, timestamp=%" PRIu64 "\n", cAck->status, tc->someInt, cAck->timestamp);
}

void ReceiveMessage(void* context, ib_EthernetController* controller, ib_EthernetMessage* metaData)
{
    TransmitContext* txContext = (TransmitContext*)(context);
    unsigned int i;
    printf(">> Ethernet Message: timestamp=%" PRIu64 "\n", metaData->timestamp);
    if (txContext != NULL)
    {
        printf("transmitContext=%d ", txContext->someInt);
    }

    printf(": ");

    for (i = PAYLOAD_OFFSET; i < metaData->ethernetFrame->size; i++)
    {
        char ch = metaData->ethernetFrame->pointer[i];
        if (isalnum(ch))
        {
            printf("%c", ch);
        }
        else
        {
            printf("<%x>", metaData->ethernetFrame->pointer[i]);
        }
    }
    printf("\n");
}

void SendEthernetMessage()
{
    uint8_t buffer[100];

    // set destination mac
    uint8_t destinationMac[6] = { 0xF6, 0x04, 0x68, 0x71, 0xAA, 0xC1 };
    memcpy(&(buffer[0]), destinationMac, sizeof(destinationMac));

    // set source mac
    uint8_t sourceMac[6] = { 0xF6, 0x04, 0x68, 0x71, 0xAA, 0xC2 };
    memcpy(&(buffer[6]), sourceMac, sizeof(sourceMac));

    // set ethertype
    buffer[12] = 0x00;
    buffer[13] = 0x08;

    // set payload
    ethernetMessageCounter += 1;
    size_t payloadSize = snprintf((char*)buffer + PAYLOAD_OFFSET, sizeof(buffer) - PAYLOAD_OFFSET, "ETHERNET %i", ethernetMessageCounter);

    ib_EthernetFrame ef = {(const uint8_t* const) buffer, PAYLOAD_OFFSET + payloadSize};

    transmitContext.someInt = ethernetMessageCounter;
    ib_EthernetController_SendFrame(ethernetController1, &ef, (void*)&transmitContext);
    
    printf("Ethernet Message sent \n");
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("usage: IbDemoCEthernet <ConfigJsonFile> <ParticipantName> [<DomainId>]\n");
        return 1;
    }

    char* jsonString = LoadFile(argv[1]);
    if (jsonString == NULL)
    {
        printf("Error: cannot open config file %s\n", argv[1]);
        return 1;
    }
    participantName = argv[2]; 

    const char* domainId = "42";
    if (argc >= 4)
    {
        domainId = argv[3]; 
    }

    ib_ReturnCode returnCode;
    returnCode = ib_SimulationParticipant_create(&participant, jsonString, participantName, domainId);
    if (returnCode) 
    {
        printf("%s\n", ib_GetLastErrorString());
        return 2;
    }
    printf("Creating Participant %s for simulation '%s'\n", participantName, domainId);


    returnCode = ib_EthernetController_create(&ethernetController1, participant, "ETH0");
    returnCode = ib_EthernetController_create(&ethernetController2, participant, "ETH1");

    ib_EthernetController_RegisterFrameAckHandler(ethernetController1, NULL, &AckCallback);
    ib_EthernetController_RegisterReceiveMessageHandler(ethernetController2, NULL, &ReceiveMessage);

    for (int i = 0; i < 10; i ++) 
    {
        SendEthernetMessage();
        SleepMs(1000);
    }

    ib_SimulationParticipant_destroy(participant);
    if (jsonString)
    {
        free(jsonString);
    }

    return EXIT_SUCCESS;
}