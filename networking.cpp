#include "networking.h"
#include "Arduino.h"



Network *Network::singleton = nullptr;
void handlePackag(packet);

Network::Network(short myMachine, short myGroup, short myDevice)
{
    me = {myMachine, myGroup, myDevice};
    I2CP = new I2C(me, ENTITYTOI2CADDR(me));
    SERP = new SerialProcessor(me, 9600);

    I2CP->onReceiveData(handlePackag);
    SERP->onReceiveData(handlePackag);

    Network::singleton = this;
}

void Network::registerFunction(packet_handler handler, short index)
{ vectors[index] = handler; }
void Network::deRegisterFunction(short index)
{ vectors[index] = nullptr; }
void Network::processPackage(packet data)
{
    if(vectors[data.to.mode] != nullptr) {vectors[data.to.mode](data);} else {vectors[0](data);}
}

void Network::sendWithI2C(short localDevice, const char *message)
{ I2CP->setupPacket( {me.machine, me.group, localDevice, 0} ); I2CP->encodeMessage(message); I2CP->sendPacket(); }
void Network::sendWithI2C(short localDevice, short mode, const char *message)
{ I2CP->setupPacket( {me.machine, me.group, localDevice, mode} ); I2CP->encodeMessage(message); I2CP->sendPacket(); }
void Network::sendWithI2C(entity k, short mode, const char *message)
{ k.mode = mode; I2CP->setupPacket( k ); I2CP->encodeMessage(message); I2CP->sendPacket(); }
void Network::sendWithI2C(entity target, const char *message)
{ I2CP->setupPacket( target ); I2CP->encodeMessage(message); I2CP->sendPacket(); }
void Network::sendWithI2C(entity target, short data)
{ I2CP->setupPacket( target ); I2CP->encodeMessage(data); I2CP->sendPacket(); }

PayloadProcessor::PayloadProcessor(entity _for)
{
    this->owner = _for;
    this->packetToBeSent = nullptr;
    this->onReceive = nullptr;
    //Serial.print("new payload processor!");
    this->initialize();
}
void PayloadProcessor::setupPacket()
{

    //Serial.println("Creating new packet.");

    // If the packet already exists, flush it.
    //if(this->packetToBeSent != nullptr)
    //{
    //    if(this->packetToBeSent->payloadV != nullptr)
    //        free(this->packetToBeSent->payloadV);
    //    delete(this->packetToBeSent);
    //}

    //Serial.println("New packet created.");

    this->packetToBeSent = new packet();
    this->packetToBeSent->payloadV = nullptr;
}
void PayloadProcessor::setupPacket(entity to)
{
    setupPacket();
    this->packetToBeSent->from = owner;
    this->packetToBeSent->to = to;
}
void PayloadProcessor::encodeMessage(const char *message)
{
    // Only encode message if packet exists.
    if(packetToBeSent == nullptr)
        return;

    //Serial.println("Message exists");

    // Initialize values
    int c = 0;
    char *vRead = (char *)message;
    char *v = 0;

    // Get string size
    while(*vRead)
    {
        c++;
        vRead++;
    }

    //Serial.print("The count is: ");
    //Serial.println(c);

    // Copy it into a new buffer
    v = (char *)calloc(1 + c, sizeof(char));
    for(int i = 0; i < c; i++)
        v[i+1] = message[i];
    v[0] = c;

    //Serial.println("Copied to buffer");
    
    packetToBeSent->payloadV = v;
    packetToBeSent->payloadC = c;

    //Serial.println("message encoded");

    // This means that:
    /* [c]:[v]                                    */
    /* ^^^^^^^ -> msg                             */
    /* PayloadC counts only the contents of 'V'!! */
    // Meaning that the amount of data stored in 'v' is actually c + 1
}
void PayloadProcessor::encodeMessage(short data)
{
    packetToBeSent->payloadV = (char *)calloc(sizeof(short)/sizeof(char), sizeof(char));
    packetToBeSent->payloadV[0] = data;
    packetToBeSent->payloadC = 1;
}
packet *PayloadProcessor::PreparePacket()
{
    packetToBeSent->sentBy = owner;
    return packetToBeSent;
}
int PayloadProcessor::packetsAvailable()
{ return this->packetBufferC; }
packet PayloadProcessor::readPacket()
{
    // Check if there is anything. If there is not, return empty packet
    if(packetBufferC == 0)
        return packet();

    // If there is something, return it from the stack and create copy
    packet copy = packet(*packetBuffer[packetBufferC-1]);

    // Now that there is a copy, flush it. Since the handler is unkown, assume the payload is not always instanced.
    if(packetBuffer[packetBufferC-1]->payloadV != nullptr)
        free(packetBuffer[packetBufferC-1]->payloadV);
    delete(packetBuffer[packetBufferC-1]);

    // Decrement the bufferC by one;
    packetBufferC--;
    
    // Return that packet
    return copy;
}
void PayloadProcessor::registerPacket(packet data)
{
    // Simply create copy of packet
    packetBuffer[packetBufferC] = new packet(data);

    // Since we cannot rely on the sender to ensure safe payloadV contents, copy payload
    if(data.payloadV == nullptr)
    {
        packetBuffer[packetBufferC]->payloadV == nullptr;
        return;
    }

    // Copy data
    char *payload = (char *)calloc(data.payloadC + 1, sizeof(char));
    payload[0] = data.payloadC;
    for(int i = 0; i < data.payloadC; i++)
        payload[i+1] = data.payloadV[i];

    // Set data in copied payload to copied data
    packetBuffer[packetBufferC]->payloadV = payload;
    packetBuffer[packetBufferC]->payloadC = data.payloadC;

    // Increment buffer by one
    packetBufferC++;

    // If an interrupt is registered, trigger that interrupt
    if(onReceive != nullptr)
    {
        onReceive(readPacket());
    }
}
void PayloadProcessor::sendPacket(packet *packet)
{
    send(packet);
}
void PayloadProcessor::sendPacket()
{
    send(PreparePacket());
    free(packetToBeSent->payloadV);
    delete(packetToBeSent);
}
packet *PayloadProcessor::parsePacketFromBuffer(char *msg, int c)
{
    // Now that we have the full thing, decode the message.
    // We know the structure
    // machine, group, device, mode. 2 bytes * 4.
    // from, to, sender. 8 bytes * 3.

    // We also now know that if the packet is less than 24 bytes, the packet is invalid
    if(c < 12)
    {
        free(msg);
        return;
    }

    entity from = entity();
    entity to = entity();
    entity sender = entity();

    // Get the data for the 'from'
    from.machine = msg[0];
    from.group = msg[1];
    from.device = msg[2];
    from.mode = msg[3];

    // Get the data for the 'to'
    to.machine = msg[4];
    to.group = msg[5];
    to.device = msg[6];
    to.mode = msg[7];

    // Get the data for the 'sender'
    sender.machine = msg[8];
    sender.group = msg[9];
    sender.device = msg[10];
    sender.mode = msg[11];

    // now since we have this data, we have enough for a packet!
    packet *k = new packet();
    k->from = from;
    k->to = to;
    k->sentBy = sender;

    // now to decode the message.
    // since we know (or should at least expect) msg[12] to be the message count, that will be the message count.

    // now we also know that the payload should be 24 bytes + 2 bytes + payloadC bytes.
    if(msg[12] > 12 + c)
        return;
    
    // Copy over message
    k->payloadC = msg[12];
    k->payloadV = (char *)calloc(msg[12] + 1, sizeof(char));
    for(int i = 0; i < c; i++)
        { 
            k->payloadV[i+1] = msg[13+i];
        }
    k->payloadV[0] = msg[12];

    // now return that son of a bitch
    return k;
}

void handlePackag(packet data)
{
    Network::singleton->processPackage(data);
}
