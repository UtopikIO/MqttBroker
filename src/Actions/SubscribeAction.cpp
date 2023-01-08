#include "MqttBroker/MqttBroker.h"
using namespace EmbeddedMqttBroker;
SubscribeAction::SubscribeAction(MqttClient *mqttClient, ReaderMqttPacket readedPacket) : Action(mqttClient)
{
  subscribeMqttMessage = new SubscribeMqttMessage(readedPacket);
}
SubscribeAction::~SubscribeAction()
{
  delete subscribeMqttMessage;
}

void SubscribeAction::doAction()
{
  mqttClient->subscribeToTopic(subscribeMqttMessage);
}