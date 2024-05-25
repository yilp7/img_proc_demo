#include "laser.h"

Laser::Laser(int index, uint laser_type) :
    ControlPort{index},
    laser_type(laser_type)
{

}

Laser::~Laser()
{

}

#if ENABLE_PORT_JSON
nlohmann::json Laser::to_json()
{
    return nlohmann::json{
        {"type", laser_type},
        };
}

int Laser::laser_control(QString msg)
{
    communicate(msg.toLatin1(), 0, 100);

    QThread::msleep(100);

    return 0;
}
#endif

#if ENABLE_PORT_JSON
void Laser::load_from_json(const nlohmann::json &j)
{
    std::cout << j << std::endl;
    std::cout.flush();
}
#endif
