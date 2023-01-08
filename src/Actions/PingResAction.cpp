#include "MqttBroker/MqttBroker.h"
using namespace EmbeddedMqttBroker;
PingResAction::PingResAction(MqttClient *mqttClient) : Action(mqttClient)
{
}

void PingResAction::doAction()
{
  mqttClient->sendPingRes();
}