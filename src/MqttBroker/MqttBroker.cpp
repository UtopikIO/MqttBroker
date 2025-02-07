#include "MqttBroker.h"

using namespace EmbeddedMqttBroker;

MqttBroker::~MqttBroker()
{
  // delete listenerTask
  newClientListenerTask->stopListen();
  delete newClientListenerTask;

  // delete freeMqttClientTask
  freeMqttClientTask->stop();
  delete freeMqttClientTask;

  // delete all MqttClients
  std::vector<MqttClient *>::iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    delete *it;
  }

  // delete trie
  delete topicTrie;
}

MqttBroker::MqttBroker(uint16_t port)
{
  this->port = port;
  this->maxNumClients = MAXNUMCLIENTS;
  topicTrie = new Trie();

  /************* setup queues ***************************/
  deleteMqttClientQueue = xQueueCreate(1, sizeof(std::string *));
  if (deleteMqttClientQueue == NULL)
  {
    log_e("Fail to create queues");
    ESP.restart();
  }

  /************ setup Tasks *****************************/
  this->newClientListenerTask = new NewClientListenerTask(this, port);
  this->newClientListenerTask->setCore(0);
  this->freeMqttClientTask = new FreeMqttClientTask(this, &deleteMqttClientQueue);
  this->freeMqttClientTask->setCore(1);
}

void MqttBroker::addNewMqttClient(WiFiClient tcpClient, ConnectMqttMessage connectMessage)
{
  String clientId = connectMessage.getClientId();

  auto it = find_if(clients.begin(), clients.end(), [&clientId](MqttClient *obj)
                    { return obj->getId() == clientId; });

  if (it != clients.end())
  {
    log_w("Client %s already exist.", clientId.c_str());
    deleteMqttClient(clientId);
  }

  MqttClient *mqttClient = new MqttClient(tcpClient, &deleteMqttClientQueue, connectMessage.getClientId(), connectMessage.getKeepAlive(), this);
  mqttClient->startTcpListener();

  clients.push_back(mqttClient);

  log_i("New client added: %s", mqttClient->getId().c_str());
  log_i("%i clients active", clients.size());
}

void MqttBroker::deleteMqttClient(String clientId)
{
  log_i("Deleting client: %s", clientId.c_str());

  auto it = find_if(clients.begin(), clients.end(), [&clientId](MqttClient *obj)
                    { return obj->getId() == clientId.c_str(); });

  if (it == clients.end())
  {
    log_e("Client %s not found", clientId.c_str());
    return;
  }

  clients.erase(it);
  delete *it;
  log_v("Client %s deleted", clientId.c_str());
  log_i("%i clients active", clients.size());
}

void MqttBroker::startBroker()
{
  newClientListenerTask->start();
  freeMqttClientTask->start();
}

void MqttBroker::stopBroker()
{
  newClientListenerTask->stopListen();
}

void MqttBroker::publishMessage(PublishMqttMessage *publishMqttMessage)
{
  std::vector<MqttClient *> *clientsSubscribedClients = topicTrie->getSubscribedMqttClients(publishMqttMessage->getTopic().getTopic());

  log_v("Publishing to topic %s", publishMqttMessage->getTopic().getTopic().c_str());

  for (auto &client : *clientsSubscribedClients)
  {
    client->publishMessage(publishMqttMessage);
  }

  // for (std::size_t it = 0; it != clientsSubscribedClients->size(); it++)
  // {
  //   // String clientId = clientsSubscribedClients->at(it);
  //   // auto clientIt = find_if(clients.begin(), clients.end(), [&clientId](MqttClient *obj)
  //   //                         { return obj->getId() == clientId; });
  //   *it.publishMessage(publishMqttMessage);
  // }
  delete clientsSubscribedClients; // topicTrie->getSubscirbedMqttClient() don't free std::vector*
                                   // the user is responsible to free de memory allocated
}

void MqttBroker::subscribeClientToTopic(SubscribeMqttMessage *subscribeMqttMessage, MqttClient *client)
{
  std::vector<MqttTopic> topics = subscribeMqttMessage->getTopics();
  NodeTrie *node;
  for (auto &topic : topics)
  {
    node = topicTrie->subscribeToTopic(topic.getTopic(), client);
    client->addNode(node);
    log_i("Client %s subscribed to %s.", client->getId().c_str(), topic.getTopic().c_str());
  }
}