#ifndef __io_h__
#define __io_h__

#include <DHTesp.h>
#include <Ticker.h>

#define P(sem) xSemaphoreTake((sem), portMAX_DELAY)
#define V(sem) xSemaphoreGive(sem)

class myDHT
{
    DHTesp dht_obj;
    int pin;
    DHTesp::DHT_MODEL_t model;
    /** Task handle for the light value read task */
    TaskHandle_t tempTaskHandle;
    Ticker tempTicker;

    int temp, hum;
    SemaphoreHandle_t mutex;
public:
    myDHT(int p, DHTesp::DHT_MODEL_t m = DHTesp::DHT22);
    ~myDHT() = default;

    inline int get_pin(void) { return pin; }
    void resume(void) { if (tempTaskHandle) xTaskResumeFromISR(tempTaskHandle); }

    void update_data();
    inline int get_temp() { int r; P(mutex); r = temp; V(mutex); return r; }
    inline int get_hum() { int r; P(mutex); r = hum; V(mutex); return r; }
};

#endif