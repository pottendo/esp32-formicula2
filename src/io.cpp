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

#if 0
DHTesp dht1, dht2;

void tempTask(void *pvParameters);
bool getTemperature(DHTesp &d);
void triggerGetTemp();

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Comfort profile */
ComfortState cf;
/** Flag if task should run */
bool tasksEnabled = false;
/** Pin number for DHT11 data pin */
int dhtPin1 = 25;
int dhtPin2 = 26;

/**
 * initTemp
 * Setup DHT library
 * Setup task and timer for repeated measurement
 * @return bool
 *    true if task and timer are started
 *    false if task or timer couldn't be started
 */
bool initTemp() {
  byte resultValue = 0;
  // Initialize temperature sensor
	dht1.setup(dhtPin1, DHTesp::DHT22);
	dht2.setup(dhtPin2, DHTesp::DHT22);
	Serial.println("DHT initiated");

  // Start task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,                       /* Function to implement the task */
			"tempTask ",                    /* Name of the task */
			4000,                           /* Stack size in words */
			NULL,                           /* Task input parameter */
			5,                              /* Priority of the task */
			&tempTaskHandle,                /* Task handle. */
			1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) {
    Serial.println("Failed to start task for temperature update");
    return false;
  } else {
    // Start update of environment data every 20 seconds
    tempTicker.attach(2, triggerGetTemp);
  }
  return true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
	   xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) // tempTask loop
  {
    if (tasksEnabled) {
      // Get temperature values
		getTemperature(dht1);
		getTemperature(dht2);
		}
    // Got sleep again
		vTaskSuspend(NULL);
	}
}

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
*/
bool getTemperature(DHTesp &dht) {
	// Reading temperature for humidity takes about 250 milliseconds!
	// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
	// Check if any reads failed and exit early (to try again).
	if (dht.getStatus() != 0) {
		Serial.println("DHT11 error status: " + String(dht.getStatusString()));
		return false;
	}

	float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };

  Serial.print("DHT" + String(dht.getPin()));
  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
	return true;
}
#define RELAY1 12
#define RELAY4 14

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("DHT ESP32 example with tasks");
  initTemp();
  // Signal end of setup() to tasks
  tasksEnabled = true;

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY4, OUTPUT);
}

void loop() {
  if (!tasksEnabled) {
    // Wait 2 seconds to let system settle down
    delay(2000);
    // Enable task that will read values from the DHT sensor
    tasksEnabled = true;
    if (tempTaskHandle != NULL) {
			vTaskResume(tempTaskHandle);
		}
  }
  Serial.println("Setting relais: " + String(RELAY1));
  digitalWrite(RELAY1,LOW);
  //Wait2seconds
  delay(2000);
  //TurnsRelayOff
  digitalWrite(RELAY1,HIGH);
  delay(2000);

  Serial.println("Setting relais: " + String(RELAY4));
  //TurnsONRelays4
  digitalWrite(RELAY4,LOW);
  //Wait2seconds
  delay(2000);
  //TurnsRelayOff
  digitalWrite(RELAY4,HIGH);
  delay(2000);
  yield();
}
#endif