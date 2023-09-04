#include "networking.h"
#include "Arduino.h"


I2CProcessor *I2CProcessor::singleton = nullptr;

void onRec(int how_many)
{
    //Serial.println("received data");
    I2CProcessor::singleton->I2COnReceive(how_many);
    Serial.println("Receiving");
}
void I2CProcessor::I2COnReceive(int how_many)
{
    // Create a buffer for the message
    char *msg = (char *)calloc(how_many, sizeof(char));

    // Get the message
    int c = 0;
    //Serial.print("---------\nCount: ");
    //Serial.println(how_many);
    while(Wire.available())
    {
        char cc = Wire.read();
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
void I2CProcessor::send(packet *p)
{
    // create the buffer
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

    Serial.print("Sending 0x"); Serial.print(p->payloadC, HEX);
    Serial.print(" Bytes to ");
    Serial.print(p->to.machine); Serial.print('.');
    Serial.print(p->to.group); Serial.print('.');
    Serial.print(p->to.device); Serial.print('.');
    Serial.println(p->to.mode);

    Serial.print("As "); Serial.println(ENTITYTOI2CADDR(p->from));

    // Set the message count, and receive the data
    msg[12] = p->payloadC;

    //Serial.println("Sending packet. Count is:");
    //Serial.println(c);
    //Serial.println(p->payloadV);

    for(int i = 13; i < c; i++)
        msg[i] = p->payloadV[i-12];

    //Serial.print("Sending from "); Serial.print(msg[0], HEX); Serial.print('.'); Serial.print(msg[1], HEX); Serial.print('.'); Serial.print(msg[2],HEX); Serial.print('.'); Serial.println(msg[3],HEX);
    //Serial.print("Sending to   "); Serial.print(msg[4], HEX); Serial.print('.'); Serial.print(msg[5], HEX); Serial.print('.'); Serial.print(msg[6],HEX); Serial.print('.'); Serial.println(msg[7],HEX);
    //Serial.print("Sending through"); Serial.print(msg[8], HEX); Serial.print('.'); Serial.print(msg[9], HEX); Serial.print('.'); Serial.print(msg[10],HEX); Serial.print('.'); Serial.println(msg[11],HEX);
    // Now we have the whole thing decoded.
    // Send it chief.

    Serial.print("Sending packet to ");
    Serial.println(ENTITYTOI2CADDR(p->to));

    Wire.beginTransmission(ENTITYTOI2CADDR(p->to));
    Wire.write(msg, c);
    int status = Wire.endTransmission();
    
    Serial.print("Packet sent. Status: ");
    Serial.println(status);

    //Serial.println(Wire.endTransmission());
    //Serial.println(msg);

    free(msg);

}
void I2CProcessor::onReceiveData(packet_handler h)
{ this->onReceive = h; }
void I2CProcessor::initialize()
{
    Wire.begin(busaddress);
    I2CProcessor::singleton = this;
    Wire.onReceive(onRec);
}