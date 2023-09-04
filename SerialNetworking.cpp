#include "networking.h"
#include "Arduino.h"


SerialProcessor *SerialProcessor::singleton = nullptr;

void SerialProcessor::send(packet *p)
{
    int c = 13 + p->payloadC;
    char *msg = (char *)calloc(c, sizeof(char));

    // set the header data
    msg[0] = p->from.machine;
    msg[1] = p->from.group;
    msg[2] = p->from.device;
    msg[3] = p->from.mode;

    msg[4] = p->to.machine;
    msg[5] = p->to.group;
    msg[6] = p->to.device;
    msg[7] = p->to.mode;

    msg[8] = p->sentBy.machine;
    msg[9] = p->sentBy.group;
    msg[10] = p->sentBy.device;
    msg[11] = p->sentBy.mode;

    // Set the message count, and receive the data
    msg[12] = p->payloadC;

    for(int i = 13; i < c; i++)
        msg[i] = p->payloadV[i-12];

    for(int i = 0; i < c; i++)
        Serial.print(msg[i]);
    
    Serial.println();

    free(msg);
}

void SerialProcessor::receiveAll()
{

    if(!Serial.available())
        return;

    int len = Serial.available();

    char *msg = (char *)calloc(len, sizeof(char));

    // Get the message
    int c = 0;
    //Serial.print("---------\nCount: ");
    //Serial.println(how_many);
    while(Serial.available())
    {
        char cc = Serial.read();
        //Serial.print(cc);
        msg[c++] = cc;
    }
    //Serial.println("---------");

    // Parse this buffer into a packet
    packet *incoming = PayloadProcessor::parsePacketFromBuffer(msg, c);

    for(int i = 0; i < c; i++)
    {
        msg[i] = 0;
    }
    free(msg);
#ifdef REGISTERONUNKOWN
    if(onReceive == nullptr)
        registerPacket(*incoming);
    else
    {
        onReceive(*incoming);
        free(incoming->payloadV);
        delete(incoming);
    }
#else
    if(onReceive != nullptr)
    {
        onReceive(*incoming);
        free(incoming->payloadV);
        delete(incoming);
    }
#endif
}

void SerialProcessor::onReceiveData(packet_handler h)
{ this->onReceive = h; }
void SerialProcessor::initialize()
{
    SerialProcessor::singleton = this;
}