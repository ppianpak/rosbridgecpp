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

#include <functional>
#include <thread>

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

class RosbridgeWsClient
{
  std::string server_port_path;
  std::unordered_map<std::string, WsClient*> client_map;

  void start(const std::string& client_name, WsClient* client, const std::string& command, const std::function<void(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)>& callback, bool persist_connection = true, bool one_message = true)
  {
    if (strcmp(client->on_open.target_type().name(), "v") == 0)
    {
#ifdef DEBUG
      client->on_open = [client_name, command, persist_connection](std::shared_ptr<WsClient::Connection> connection) {
#else
      client->on_open = [command, persist_connection](std::shared_ptr<WsClient::Connection> connection) {
#endif

#ifdef DEBUG
        std::cout << "Client " << client_name << ": Opened connection" << std::endl;
        std::cout << "Client " << client_name << ": Sending command: \"" << command << "\"" << std::endl;
#endif
        auto send_stream = std::make_shared<WsClient::SendStream>();
        *send_stream << command;
        connection->send(send_stream);

        if (!persist_connection)
        {
          connection->send_close(1000);
        }
      };
    }

    if (strcmp(client->on_message.target_type().name(), "v") == 0)
    {
      if (callback)
      {
        client->on_message = callback;
      }
      else
      {
#ifdef DEBUG
        client->on_message = [client_name, one_message](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message) {
#else
        client->on_message = [one_message](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message) {
#endif

#ifdef DEBUG
          std::cout << "Client " << client_name << ": Message received: \"" << message->string() << "\"" << std::endl;
#endif

          if (one_message)
          {
#ifdef DEBUG
            std::cout << "Client " << client_name << ": Sending close connection" << std::endl;
#endif
            connection->send_close(1000);
          }
        };
      }
    }

    if (strcmp(client->on_close.target_type().name(), "v") == 0)
    {
#ifdef DEBUG
      client->on_close = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, int status, const std::string & /*reason*/) {
#else
      client->on_close = [](std::shared_ptr<WsClient::Connection> /*connection*/, int status, const std::string & /*reason*/) {
#endif

#ifdef DEBUG
        std::cout << "Client " << client_name << ": Closed connection with status code " << status << std::endl;
#endif
      };
    }

    if (strcmp(client->on_error.target_type().name(), "v") == 0)
    {
      // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
#ifdef DEBUG
      client->on_error = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
#else
      client->on_error = [](std::shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
#endif

#ifdef DEBUG
        std::cout << "Client " << client_name << ": Error: " << ec << ", error message: " << ec.message() << std::endl;
#endif
      };
    }

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

      start(client_name, it->second, "{" + message + "}", NULL, true);
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

      start(client_name, it->second, "{" + message + "}", NULL, false);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void subscribe(const std::string& client_name, const std::string& topic, const std::function<void(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)>& callback, const std::string& id = "", const std::string& type = "", int throttle_rate = -1, int queue_length = -1, int fragment_size = -1, const std::string& compression = "")
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

      start(client_name, it->second, "{" + message + "}", callback, true, false);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void callService(const std::string& client_name, const std::string& service, const std::function<void(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)>& callback, const std::string& id = "", const std::list<rapidjson::Document>& args = {}, int fragment_size = -1, const std::string& compression = "")
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
        // TODO
      }
      if (fragment_size > -1)
      {
        message += ", \"fragment_size\":" + std::to_string(fragment_size);
      }
      if (compression.compare("none") == 0 || compression.compare("png") == 0)
      {
        message += ", \"compression\":\"" + compression + "\"";
      }

      start(client_name, it->second, "{" + message + "}", callback, true, true);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void advertiseService(const std::string& client_name, const std::string& service, const std::string& type, const std::function<void(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::Message> message)>& callback)
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"advertise_service\", \"service\":\"" + service + "\", \"type\":\"" + type + "\"";

      start(client_name, it->second, "{" + message + "}", callback, true, false);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void serviceResponse(const std::string& client_name, const std::string& service, bool result, const std::string& id = "", const std::list<rapidjson::Document>& values = {})
  {
    std::unordered_map<std::string, WsClient*>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"service_response\", \"service\":\"" + service + "\", \"result\":" + ((result)? "true" : "false");

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }
      if (values.size() > 0)
      {
        // TODO
      }

      start(client_name, it->second, "{" + message + "}", NULL, false);
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
