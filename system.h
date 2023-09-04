#ifndef system_h
#define system_h


enum systemMode
{
    nominal,
    maintenance,
    direct
};


class System
{

    public:
        System(unsigned int group, unsigned int id);

        void performSystemCheck();
        void perfromI2CSystemCheck();
        void performUSBSystemCheck();

        void registerCommunication();

        void switchMode();
        

    private:
        unsigned int group;
        unsigned int id;


};




#endif