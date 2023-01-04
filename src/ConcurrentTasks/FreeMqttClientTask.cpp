#include "MqttBroker/MqttBroker.h"
using namespace EmbeddedMqttBroker;
/********************** FreeMqttClientTask *************************/

FreeMqttClientTask::FreeMqttClientTask(MqttBroker *broker, QueueHandle_t *deleteMqttClientQueue) : Task("FreeMqttClientTask", 1024 * 2, TaskPrio_Low)
{
  this->broker = broker;
  this->deleteMqttClientQueue = deleteMqttClientQueue;
}

void FreeMqttClientTask::run(void *data)
{
  String clientId;
  while (true)
  {
    xQueueReceive((*deleteMqttClientQueue), &clientId, portMAX_DELAY);
    log_i("Deleting client: %s", clientId.c_str());
    broker->deleteMqttClient(clientId);
  }
}