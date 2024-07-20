/**
 * @author JamesAresBrown
 * @version 0.1
 * @date 2024-01-21
 * @copyright Copyright (JamesAresBrown) 2024
 */
#ifndef MYSERVER_APPLICATION_H
#define MYSERVER_APPLICATION_H

class Application {
public:
    Application() = default;
    virtual ~Application() = default;
    virtual bool Read() = 0;
    virtual bool Process() = 0;
    virtual bool Write() = 0;
private:
};


#endif //MYSERVER_APPLICATION_H
