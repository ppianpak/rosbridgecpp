/*
 *  Created on: Apr 16, 2018
 *      Author: ppianpak
 */

#include "rosbridge_ws_client.hpp"

#include <chrono>

RosbridgeWsClient rbc("localhost:9090");

void callback(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)
{
//  std::cout << "callback: " << message->string() << std::endl;
}

void callback2(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)
{
  std::cout << "callback2: " << message->string() << std::endl;
  rbc.serviceResponse("/zservice", false);
}

int main() {
  // 1. Parse a JSON string into DOM.
  const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
  rapidjson::Document d;
  d.Parse(json);

  rapidjson::Document document;
  document.SetObject();
  document.AddMember("abc", "def", document.GetAllocator());
  std::cout << document["abc"].GetString() << std::endl;

  // 3. Stringify the DOM
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  document.Accept(writer);
  std::cout << buffer.GetString() << std::endl;

  rbc.addClient("client01");
  rbc.addClient("client02");
  rbc.addClient("client03");
  rbc.addClient("client04");
  rbc.addClient("client05");
  rbc.removeClient("client05");

  rbc.advertise("client01", "/z", "std_msgs/Int32");
  rbc.advertiseService("client02", "/zservice", "std_srvs/Empty", callback2);
  rbc.subscribe("client03", "/uav0/ground_truth_to_tf/pose", callback);

  while(true)
  {
    rapidjson::Document d;
    d.SetObject();
    d.AddMember("data", 12345, d.GetAllocator());
    rbc.publish("client04", "/z", d);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }

//  std::cout << "-> Sleeping1" << std::endl;
//  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//  std::cout << "& Sleeping1" << std::endl;
//
//  rbc.stopClient("client03");
//  std::cout << "-> Sleeping2" << std::endl;
//  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//  std::cout << "& Sleeping2" << std::endl;
}
