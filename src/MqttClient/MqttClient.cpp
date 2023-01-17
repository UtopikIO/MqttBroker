#include "MqttBroker/MqttBroker.h"
using namespace EmbeddedMqttBroker;
/***************************** MqttClient class *************************/
MqttClient::~MqttClient()
{
  // delete action; // action is allocated when it is used
  // and deleted after such use.

  tcpListenerTask->stop();
  delete tcpListenerTask;

  for (int i = 0; i < nodesToFree.size(); i++)
  {
    nodesToFree[i]->unSubscribeMqttClient(this);
  }
}

MqttClient::MqttClient(WiFiClient tcpConnection, QueueHandle_t *deleteMqttClientQueue,
                       String clientId, uint16_t keepAlive, uint16_t timeout, MqttBroker *broker)
{
  this->clientId = clientId;
  this->keepAlive = keepAlive;
  this->tcpConnection = tcpConnection;
  this->deleteMqttClientQueue = deleteMqttClientQueue;
  this->timeout = timeout;
  this->broker = broker;

  this->tcpListenerTask = new TCPListenerTask(this);
  this->tcpListenerTask->setCore(1);
  lastAlive = millis();
}

void MqttClient::publishMessage(PublishMqttMessage *publishMessage)
{
  log_v("Topic %s send to %s", publishMessage->getTopic().getTopic().c_str(), this->clientId.c_str());
  log_v("\n%s", publishMessage->getTopic().getPayLoad().c_str());
  /*
  for qos > 0
  uint8_t publishFlasgs = 0x6 & topics[i].getQos();
  publishMessage->setFlagsControlType(publishFlasgs);
  */

  sendPacketByTcpConnection(publishMessage->buildMqttPacket());
}

void MqttClient::subscribeToTopic(SubscribeMqttMessage *subscribeMqttMessage)
{
  broker->subscribeClientToTopic(subscribeMqttMessage, this);
}

uint8_t MqttClient::checkConnection()
{
  unsigned long now = millis();

  if (tcpConnection.connected())
  {
    // TODO: Analyse keepalive
    if (now - lastAlive > (keepAlive * 1000) + timeout) // keepAlive (seconds)
    {
      log_e("%s didn't send ping on time.", clientId.c_str());
      log_d("%ims too late.", (now - lastAlive) - (keepAlive * 1000) + timeout);
      tcpConnection.stop();
    }

    if (tcpConnection.available())
    {
      // read mqtt packet
      ReaderMqttPacket reader;
      reader.readMqttPacket(tcpConnection);

      // get new action.
      ActionFactory factory;
      action = factory.getAction(this, reader);
      action->doAction();

      // free Action allocated memory.
      delete action;
      lastAlive = now;
    }
  }

  return tcpConnection.connected(); // If it is recived an Disconnect packet
                                    // connect status changes.
}

void MqttClient::notifyPublishRecived(PublishMqttMessage *publishMessage)
{
  broker->publishMessage(publishMessage);
}

void MqttClient::sendPacketByTcpConnection(String mqttPacket)
{
  tcpConnection.write(mqttPacket.c_str(), mqttPacket.length()); // ok!!
}

void MqttClient::sendPingRes()
{
  String resPacket = messagesFactory.getPingResMessage().buildMqttPacket();
  log_v("%s sending ping response.", this->clientId.c_str());
  sendPacketByTcpConnection(resPacket);
}

/****************************** TaskTcpListener ********************/
void MqttClient::startTcpListener()
{
  tcpListenerTask->start();
}
