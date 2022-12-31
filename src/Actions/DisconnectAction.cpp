#include "MqttBroker/MqttBroker.h"
using namespace EmbeddedMqttBroker;
DisconnectAction::DisconnectAction(MqttClient *mqttClient) : Action(mqttClient)
{
}

void DisconnectAction::doAction()
{
  mqttClient->disconnect();
}