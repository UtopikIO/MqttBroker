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
  std::map<String, MqttClient *>::iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    delete it->second;
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
  deleteMqttClientQueue = xQueueCreate(1, sizeof(int));
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

  std::map<String, EmbeddedMqttBroker::MqttClient *>::iterator it;
  it = clients.find(clientId.c_str());
  if (it != clients.end())
  {
    log_w("Client %s already exist", clientId.c_str());
    deleteMqttClient(clientId);
  }

  MqttClient *mqttClient = new MqttClient(tcpClient, &deleteMqttClientQueue, connectMessage.getClientId(), connectMessage.getKeepAlive(), this);
  mqttClient->startTcpListener();

  clients.insert(std::make_pair(connectMessage.getClientId(), mqttClient));

  log_i("New client added: %s", mqttClient->getId().c_str());
  log_i("%i clients active", clients.size());
}

void MqttBroker::deleteMqttClient(String clientId)
{
  log_i("Deleting client: %s", clientId.c_str());

  std::map<String, EmbeddedMqttBroker::MqttClient *>::iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    // log_v("%s", it->first.c_str());
    if (!strcmp(it->first.c_str(), clientId.c_str()))
    {
      MqttClient *client = it->second;
      clients.erase(it);
      delete client;
      log_v("Client %s deleted", clientId.c_str());
      log_i("%i clients active", clients.size());
      return;
    }
  }

  // Do not work
  // std::map<String, EmbeddedMqttBroker::MqttClient *>::iterator it = clients.find(clientId.c_str());
  // if (it != clients.end())
  // {
  //   log_e("Client %s not found", clientId.c_str());
  //   log_i("%i clients active", clients.size());
  //   return;
  // }

  // log_i("%i clients active", clients.size());
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
  std::vector<String> *clientsSubscribedIds = topicTrie->getSubscribedMqttClients(publishMqttMessage->getTopic().getTopic());

  log_v("Publishing to topic %s", publishMqttMessage->getTopic().getTopic().c_str());

  for (std::size_t it = 0; it != clientsSubscribedIds->size(); it++)
  {
    clients[clientsSubscribedIds->at(it)]->publishMessage(publishMqttMessage);
  }
  delete clientsSubscribedIds; // topicTrie->getSubscirbedMqttClient() don't free std::vector*
                               // the user is responsible to free de memory allocated
}

void MqttBroker::SubscribeClientToTopic(SubscribeMqttMessage *subscribeMqttMessage, MqttClient *client)
{
  std::vector<MqttTopic> topics = subscribeMqttMessage->getTopics();
  NodeTrie *node;
  for (int i = 0; i < topics.size(); i++)
  {
    log_i("Client %s subscribed to %s", client->getId().c_str(), topics[i].getTopic().c_str());
    node = topicTrie->subscribeToTopic(topics[i].getTopic(), client);
    client->addNode(node);
  }
}