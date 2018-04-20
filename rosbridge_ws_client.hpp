/*
 *  Created on: Apr 16, 2018
 *      Author: ppianpak
 */

#ifndef ROSBRIDGECPP_ROSBRIDGE_WS_CLIENT_HPP_
#define ROSBRIDGECPP_ROSBRIDGE_WS_CLIENT_HPP_

#include "Simple-WebSocket-Server/client_ws.hpp"

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"

#include <chrono>
#include <functional>
#include <thread>

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
using OnMessage =  std::function<void(std::shared_ptr<WsClient::Connection>, std::shared_ptr<WsClient::Message>)>;

class RosbridgeWsClient
{
  std::string server_port_path;
  std::unordered_map<std::string, WsClient*> client_map;

  void start(const std::string& client_name, WsClient* client, const std::string& message)
  {
    if (!client->on_open)
    {
#ifdef DEBUG
      client->on_open = [client_name, message](std::shared_ptr<WsClient::Connection> connection) {
#else
      client->on_open = [message](std::shared_ptr<WsClient::Connection> connection) {
#endif

#ifdef DEBUG
        std::cout << "Client " << client_name << ": Opened connection" << std::endl;
        std::cout << "Client " << client_name << ": Sending message: \"" << message << "\"" << std::endl;
#endif
        auto send_stream = std::make_shared<WsClient::SendStream>();
        *send_stream << message;
        connection->send(send_stream);
      };
    }

#ifdef DEBUG
    if (!client->on_message)
    {
      client->on_message = [client_name](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message) {
        std::cout << "Client " << client_name << ": Message received: \"" << message->string() << "\"" << std::endl;
      };
    }

    if (!client->on_close)
    {
      client->on_close = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, int status, const std::string & /*reason*/) {
        std::cout << "Client " << client_name << ": Closed connection with status code " << status << std::endl;
      };
    }

    if (!client->on_error)
    {
      // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
      client->on_error = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        std::cout << "Client " << client_name << ": Error: " << ec << ", error message: " << ec.message() << std::endl;
      };
    }
#endif

#ifdef DEBUG
    std::thread client_thread([client_name, client]() {
#else
    std::thread client_thread([client]() {
#endif
      client->start();

#ifdef DEBUG
      std::cout << "Client " << client_name << ": Terminated" << std::endl;
#endif
      client->on_open = NULL;
      client->on_message = NULL;
      client->on_close = NULL;
      client->on_error = NULL;
    });

    client_thread.detach();

    // This is to make sure that the thread got fully launched before we do anything to it (e.g. remove)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

public:
  RosbridgeWsClient(const std::string& server_port_path)
  {
    this->server_port_path = server_port_path;
  }

  ~RosbridgeWsClient()
  {
    for (const auto &client : client_map)
    {
      client.second->stop();
      delete client.second;
    }
  }

  void addClient(const std::string& client_name)
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it == client_map.end())
    {
      client_map[client_name] = new WsClient(server_port_path);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << " has already been created" << std::endl;
    }
#endif
  }

  WsClient* getClient(const std::string& client_name)
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      return it->second;
    }
    return NULL;
  }

  void stopClient(const std::string& client_name)
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      it->second->stop();
      it->second->on_open = NULL;
      it->second->on_message = NULL;
      it->second->on_close = NULL;
      it->second->on_error = NULL;
#ifdef DEBUG
      std::cout << client_name << " has been stopped" << std::endl;
#endif
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << " has not been created" << std::endl;
    }
#endif
  }

  void removeClient(const std::string& client_name)
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      it->second->stop();
      client_map.erase(it);
      delete it->second;
#ifdef DEBUG
      std::cout << client_name << " has been removed" << std::endl;
#endif
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << " has not been created" << std::endl;
    }
#endif
  }

  void advertise(const std::string& client_name, const std::string& topic, const std::string& type, const std::string& id = "")
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"advertise\", \"topic\":\"" + topic + "\", \"type\":\"" + type + "\"";

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }

      start(client_name, it->second, "{" + message + "}");
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void publish(const std::string& client_name, const std::string& topic, const rapidjson::Document& msg, const std::string& id = "")
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      msg.Accept(writer);

      std::string message = "\"op\":\"publish\", \"topic\":\"" + topic + "\", \"msg\":" + strbuf.GetString();

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }
      message = "{" + message + "}";

#ifdef DEBUG
      it->second->on_open = [client_name, message](std::shared_ptr<WsClient::Connection> connection) {
#else
      it->second->on_open = [message](std::shared_ptr<WsClient::Connection> connection) {
#endif

#ifdef DEBUG
        std::cout << "Client " << client_name << ": Opened connection" << std::endl;
        std::cout << "Client " << client_name << ": Sending message: \"" << message << "\"" << std::endl;
#endif
        auto send_stream = std::make_shared<WsClient::SendStream>();
        *send_stream << message;
        connection->send(send_stream);

        // TODO: This could be improved by creating a thread to keep publishing the message instead of closing it right away
        connection->send_close(1000);
      };

      start(client_name, it->second, message);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void subscribe(const std::string& client_name, const std::string& topic, const OnMessage& callback, const std::string& id = "", const std::string& type = "", int throttle_rate = -1, int queue_length = -1, int fragment_size = -1, const std::string& compression = "")
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"subscribe\", \"topic\":\"" + topic + "\"";

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }
      if (type.compare("") != 0)
      {
        message += ", \"type\":\"" + type + "\"";
      }
      if (throttle_rate > -1)
      {
        message += ", \"throttle_rate\":" + std::to_string(throttle_rate);
      }
      if (queue_length > -1)
      {
        message += ", \"queue_length\":" + std::to_string(queue_length);
      }
      if (fragment_size > -1)
      {
        message += ", \"fragment_size\":" + std::to_string(fragment_size);
      }
      if (compression.compare("none") == 0 || compression.compare("png") == 0)
      {
        message += ", \"compression\":\"" + compression + "\"";
      }

      it->second->on_message = callback;
      start(client_name, it->second, "{" + message + "}");
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void advertiseService(const std::string& client_name, const std::string& service, const std::string& type, const OnMessage& callback)
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"advertise_service\", \"service\":\"" + service + "\", \"type\":\"" + type + "\"";

      it->second->on_message = callback;
      start(client_name, it->second, "{" + message + "}");
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void serviceResponse(const rapidjson::Value& service_request, bool result, const std::vector<rapidjson::Document>& values = {})
  {
    std::string message = "\"op\":\"service_response\", \"service\":\"" + std::string(service_request["service"].GetString()) + "\", \"result\":" + ((result)? "true" : "false");

    // Rosbridge somehow does not allow service_response to be sent without id and values
    message += ", \"id\":\"" + std::string(service_request["id"].GetString()) + "\"";

    message += ", \"values\":[";
    if (values.size() > 0)
    {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

      values[0].Accept(writer);
      message += std::string(buffer.GetString());

      for (int i = 1; i < values.size(); i++)
      {
        values[i].Accept(writer);
        message += ", " + std::string(buffer.GetString());
      }
    }
    message += "]";

//    TODO
//    start(client_name, it->second, "{" + message + "}");
  }

  void callService(const std::string& client_name, const std::string& service, const OnMessage& callback, const std::string& id = "", const std::vector<rapidjson::Document>& args = {}, int fragment_size = -1, const std::string& compression = "")
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"call_service\", \"service\":\"" + service + "\"";

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }
      if (args.size() > 0)
      {
        message += ", \"args\":[";

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

        args[0].Accept(writer);
        message += std::string(buffer.GetString());

        for (int i = 1; i < args.size(); i++)
        {
          args[i].Accept(writer);
          message += ", " + std::string(buffer.GetString());
        }

        message += "]";
      }
      if (fragment_size > -1)
      {
        message += ", \"fragment_size\":" + std::to_string(fragment_size);
      }
      if (compression.compare("none") == 0 || compression.compare("png") == 0)
      {
        message += ", \"compression\":\"" + compression + "\"";
      }

      if (callback)
      {
        it->second->on_message = callback;
      }
      else
      {
#ifdef DEBUG
        it->second->on_message = [client_name](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message) {
#else
        it->second->on_message = [](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message) {
#endif

#ifdef DEBUG
          std::cout << "Client " << client_name << ": Message received: \"" << message->string() << "\"" << std::endl;
          std::cout << "Client " << client_name << ": Sending close connection" << std::endl;
#endif
          connection->send_close(1000);
        };
      }

      start(client_name, it->second, "{" + message + "}");
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }
};

#endif
