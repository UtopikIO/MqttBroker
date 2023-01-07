#ifndef PUBLISHMQTTMESSAGE_H
#define PUBLISHMQTTMESSAGE_H

#include "MqttMessage.h"
#include "ReaderMqttPacket.h"
#include "MqttMessagesSerealizable.h"
#include "MqttTopicPayload.h"

/**
 * @brief Publish mqtt message, Client can sends a publish mqtt packet
 * this class abstracts how to decode this mqtt packet.
 *
 * For other side, broker needs to resend the mqtt publish packet to others clients,
 * This class abstracts how to encode and build a mqtt publish packet to
 * send by tcp connection.
 *
 */
class PublishMqttMessage : public MqttMessage, public MqttMessageSerealizable
{
private:
  MqttTopicPayload topicPayload;
  uint16_t messageId;

  /**
   * @brief Concatenate encoded size according to mqtt packet formar.
   *
   * @param encodedSize size to concatenate.
   * @param mqttPacket String where concat the encodedSize.
   * @return String with encodedSize concatenated.
   */
  String concatEncodedSize(uint32_t encodedSize, String mqttPacket);

public:
  PublishMqttMessage() : MqttMessage(PUBLISH, 0)
  {
  }
  /**
   * @brief Construct a new Publish Mqtt Message object, this method
   * create a PublishMqttMessage from raw bytes buffer readed directly
   * from a tcp connection.
   *
   * @param packetReaded object who contains bytes readed from tcp connection.
   */
  PublishMqttMessage(ReaderMqttPacket packetReaded);

  /**
   * @brief Construct a new Publish Mqtt Message object indicating
   * his publish flasgs: dup,qos,retain.
   *
   * @param publishFlags for this mqtt publish message.
   */
  PublishMqttMessage(uint8_t publishFlags);

  /**
   * @brief Construct a new Publish Mqtt Message object, with
   * a MqttTopicPayload object and publishFlags, it is used for create
   * a PublishMqttMessage that broker want to send to clients.
   *
   * @param publishFlags
   * @param topic
   */
  PublishMqttMessage(uint8_t publishFlags, MqttTopicPayload topicPayload);

  bool isTopic(MqttTopicPayload topicPayload)
  {
    return this->topicPayload.isTopic(topicPayload);
  }

  String buildMqttPacket();

  void setTopic(String topicPayload)
  {
    this->topicPayload.setTopic(topicPayload);
  }

  void setPayLoad(String payLoad)
  {
    this->topicPayload.setPayLoad(payLoad);
  }

  void setQos(uint8_t qos)
  {
    this->topicPayload.setQos(qos);
  }
  void setMessageId(uint16_t messageId)
  {
    this->messageId = messageId;
  }

  MqttTopicPayload getTopicPayload()
  {
    return topicPayload;
  }
};

#endif