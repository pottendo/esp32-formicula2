#include <list>
#include "ui.h"
#include "io.h"

/* Temperature & Humidity */
void tempTask(void *pvParameters);
/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
static void triggerGetTemp(lv_task_t *t)
{
    //  t->resume();
    myDHT *obj = static_cast<myDHT *>(t->user_data);
    obj->update_data();
}

myDHT::myDHT(int p, DHTesp::DHT_MODEL_t m)
    : pin(p), model(m), temp(-500), hum(-99)
{
    dht_obj.setup(pin, m);
    mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(mutex); // Initialize to be accessible
    lv_task_t *dht_io_task = lv_task_create(triggerGetTemp, 2000, LV_TASK_PRIO_LOW, this);
    if (!dht_io_task)
    {
        log_msg("Failed to create taks for DHT sensor.");
    }
}

void myDHT::update_data(void)
{
    
    TempAndHumidity newValues = dht_obj.getTempAndHumidity();
    if (dht_obj.getStatus() != 0)
    {
        log_msg(String("DHT (") + get_pin() + ") error status: " + String(dht_obj.getStatusString()));
        return;
    }
    P(mutex);
    temp = newValues.temperature;
    hum = newValues.humidity; // ((rand() % 100) - 50.0) / 20.0;
    V(mutex);
    //log_msg(String("DHT(") + get_pin() + "): Temp = " + String(temp) + ", Hum = " + String(hum));
    std::for_each(parents.begin(), parents.end(),
                  [&](avgDHT *p) { p->update_data(this); });
}
