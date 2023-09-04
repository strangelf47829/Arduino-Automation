// Author: Rafael de Bie
// Copyright (C) 2023-2023 Rafael de Bie

// Created for use with arduino

#ifndef networking_h
#define networking_h

#include "Arduino.h"


#define ENTITYTOI2CADDR(e)      ((0x7F - (e.machine ^ e.group)) ^ e.device)
#define ENTITYPTRTOI2CADDR(e)      ((0x7F - (e->machine ^ e->group)) ^ e->device)

// I2C interfacing
// Roll call handling
// Vector table
// Encoders
// Yada yada

/*
System follows this protocol

packets are structured as follows:
[from]:[to]:[sent]:[message]
    where:
    'from' is the sender of the packet
    'to' is the intended receiver of the packet
    'sent' is the sender of the message
    'message' is the payload

To know which device is talking to which device, the following addressing is done:
machine.group.device.mode

The network is split by machines. Each machine gets its own ID on the system.
Groups are: 'objects (virtual or otherwise) that, if isolated, would still contain a hierarchy of devices'.
If a device does not belong to a device, it defaults to group '255'
The device is the ID of the device within that network.
The mode is a subsystem of that device (device specific.)
    The mode is a virtual address: It is up to the device to configure what it means.

These values may not exceed 255. This is done because the whole ID can fit onto a single int.

The payload is just sent in plain text. No need to do anything fancy.


*/

struct entity
{
    short machine;
    short group;
    short device;
    short mode;
};
struct packet
{
    entity from;
    entity to;
    entity sentBy;

    char *payloadV;
    int payloadC;
};
using packet_handler = void (*)(packet data);

/* The payload processor is used by the networking class */
class PayloadProcessor
{
    public:
        PayloadProcessor(entity _for);

        // Packet interfacing
        void setupPacket();                         // Tells the processor to start with a new packet
        void setupPacket(entity to);                // Tells the processor to start with a new packet, and tells it to who it is for.
        void encodeMessage(const char *message);    // Encodes the message to be encoded.
        void encodeMessage(short data);
        packet *PreparePacket();                    // Finally gets the packet, and stops processing.

        // Receiving packets
        int packetsAvailable();                     // Returns the amount of packets available to be read.
        packet readPacket();                        // Get the read packet

        // Sending packets. Always fun!
        void sendPacket(packet *);                  // Send an already, pre-made packet. Totally ok! Free of charge!
        void sendPacket();                          // "10 dollar premade packet" packets at a ludicrous price?!?! Unnaceptable!

        static packet *parsePacketFromBuffer(char *, int);

    protected:
        void registerPacket(packet data);           // Called from the derived class. Tells the payload handler a new packet has arrived. Yay :D
        virtual void initialize()                   // My whole weekend is just coding. Cant the code speak for itself?
        {}
        virtual void send(packet *)                 // Tells the interface to send a packet.
        {}

        void (*onReceive)(packet data);             // The function that must be called when a packet arrives

    private:
        entity owner;                               // Who is my big daddy owo
        packet *packetToBeSent;                     // This is the packet to be sent by this handler

        packet **packetBuffer;                      // this buffer holds the packets
        unsigned int packetBufferC;                 // this int holds tells us the amount of packets available to us
};

#include <Wire.h>

// A standard class for I2C.. How handy!!
class I2CProcessor: public PayloadProcessor
{
    public:
        I2CProcessor(entity _for, int address): PayloadProcessor(_for) { this->busaddress = address; initialize(); }
        void I2COnReceive(int how_many);

        static I2CProcessor *singleton;

        void onReceiveData(packet_handler);
        
        void send(packet *);
        
        int busaddress;

    private:
        void initialize();

};

class SerialProcessor: public PayloadProcessor
{
    public:
        SerialProcessor(entity _for, int baud): PayloadProcessor(_for) {this->baud = baud; initialize(); }
        void receiveAll();

        static SerialProcessor *singleton;

        void onReceiveData(packet_handler);

        void send(packet *);

        int baud;

        private:
            void initialize();
};



/* The network class keeps track of current networking, but also provides an API to interact with it */
class Network
{
    public:
        Network(short myMachine, short myGroup, short myDevice);

        // mode handling
        void registerFunction(packet_handler, short);
        void deRegisterFunction(short);

        void processPackage(packet);
        static Network *singleton;

        void sendWithI2C(short localDevice, const char *message);
        void sendWithI2C(short localDevice, short mode, const char *message);
        void sendWithI2C(entity target, const char *message);
        void sendWithI2C(entity target, short mode, const char *message);
        void sendWithI2C(entity target, short data);

        void receiveAll()
        {
            SERP->receiveAll();
        }

        I2CProcessor *I2CP;
        SerialProcessor *SERP;
        
        entity me;
    private:
        
        packet_handler vectors[255];

        

};



using I2C = I2CProcessor;



#endif