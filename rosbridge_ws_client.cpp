/*
 *  Created on: Apr 16, 2018
 *      Author: ppianpak
 */

#include "rosbridge_ws_client.hpp"

RosbridgeWsClient rbc("localhost:9090");

void callback(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)
{
//  std::cout << "callback: " << message->string() << std::endl;
}

void callback2(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)
{
  std::cout << "callback2: " << message->string() << std::endl;

  auto send_stream = std::make_shared<WsClient::SendStream>();
  std::string m = "{\"op\":\"service_response\", \"service\":\"/zservice\", \"result\":true, \"id\":\"service_request:/zservice:1\", \"values\":{}}";
  *send_stream << m;
  connection->send(send_stream);
}


class Test { public: ~Test() { std::puts("Test destroyed."); } };

int main() {
  {
  std::shared_ptr<Test> p = std::make_shared<Test>();
  std::shared_ptr<Test> q = p;
  std::cout << "use_count: " << q.use_count() << std::endl;
//  std::puts("p.reset()...");
//  p.reset();
//  std::puts("q.reset()...");
//  q.reset();
  }
  std::puts("done");

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
  d.Accept(writer);
  std::cout << buffer.GetString() << std::endl;

  rbc.addClient("client01");
  rbc.addClient("client02");
  rbc.addClient("client03");
//  rbc.addClient("client04");
//  rbc.subscribe("client04", "/uav0/ground_truth_to_tf/pose", nullptr);
//  rbc.addClient("client05");
//  rbc.subscribe("client05", "/uav0/ground_truth_to_tf/pose", nullptr);

  rbc.advertise("client01", "/z", "std_msgs/Int32");
  rapidjson::Document dd;
  dd.SetObject();
  dd.AddMember("data", 12345, dd.GetAllocator());
  rbc.publish("client02", "/z", dd);

  std::cout << "-> Sleeping1" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  std::cout << "& Sleeping1" << std::endl;

  rbc.removeClient("client02");

  std::cout << "-> Sleeping2" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  std::cout << "& Sleeping2" << std::endl;

//  rbc.advertiseService("client02", "/zservice", "std_srvs/Empty", NULL);
//  rbc.subscribe("client03", "/uav0/ground_truth_to_tf/pose", callback);
//
//  while(true)
//  {
//    rapidjson::Document d;
//    d.SetObject();
//    d.AddMember("data", 12345, d.GetAllocator());
//    rbc.publish("client04", "/z", d);
//    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
//  }

//  rbc.stopClient("client03");
//  std::cout << "-> Sleeping2" << std::endl;
//  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//  std::cout << "& Sleeping2" << std::endl;
}
