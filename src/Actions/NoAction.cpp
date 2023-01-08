#include "MqttBroker/MqttBroker.h"
using namespace EmbeddedMqttBroker;
NoAction::NoAction(MqttClient *mqttClient) : Action(mqttClient)
{
}

void NoAction::doAction()
{
  log_i("No action");
}