#include "MqttBroker/MqttBroker.h"

using namespace EmbeddedMqttBroker;

/********************** ListenerNewClientTask ******************/

NewClientListenerTask::NewClientListenerTask(MqttBroker *broker, uint16_t port) : Task("ListenerNewClientTask", 1024 * 3, TaskPrio_Mid)
{
  this->broker = broker;
  this->tcpServer = new WiFiServer(port);
}

NewClientListenerTask::~NewClientListenerTask()
{
  tcpServer->close();
  delete tcpServer;
}

void NewClientListenerTask::run(void *data)
{
  tcpServer->begin();

  while (true)
  {
    // Check for a new mqtt client connected
    WiFiClient client = tcpServer->available();
    if (!client)
    {
      vTaskDelay(10); // yield cpu, This line is necessary for meet freeRTOS time constrains
      continue;       // next iteration
    }

    // Waiting client to send mqttpacket
    int count = 1000; // 1 sec
    for (size_t i = 0; i < count; i += 10)
    {
      if (client.available())
        break;
      vTaskDelay(100);
    }

    if (!client.available())
    {
      log_w("Client from %s rejected.", client.remoteIP().toString());
      continue;
    }

    /** reading bytes from client, in this point Broker only recive and
     acept connect mqtt packets **/
    ConnectMqttMessage connectMessage = messagesFactory.getConnectMqttMessage(client);

    if (!connectMessage.malFormedPacket() && !broker->isBrokerFullOfClients())
    {
      sendAckConnection(client);
      broker->addNewMqttClient(client, connectMessage);
    }
  }
}

void NewClientListenerTask::stopListen()
{
  this->stop();
  tcpServer->close();
}

void NewClientListenerTask::sendAckConnection(WiFiClient tcpClient)
{
  String ackPacket = messagesFactory.getAceptedAckConnectMessage().buildMqttPacket();
  sendPacketByTcpConnection(tcpClient, ackPacket);
}

void NewClientListenerTask::sendPacketByTcpConnection(WiFiClient client, String mqttPacket)
{
  uint8_t buff[mqttPacket.length()];
  mqttPacket.getBytes(buff, mqttPacket.length());
  client.write(buff, mqttPacket.length());
}
