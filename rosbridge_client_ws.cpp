/*
 * rosbridgeclient.cpp
 *
 *  Created on: Apr 15, 2018
 *      Author: ppianpak
 */

#include "Simple-WebSocket-Server/client_ws.hpp"

using namespace std;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;

int main() {
  WsClient client("localhost:9090");
  client.on_message = [](shared_ptr<WsClient::Connection> connection, shared_ptr<WsClient::Message> message) {
    cout << "Client: Message received: \"" << message->string() << "\"" << endl;

    cout << "Client: Sending close connection" << endl;
    connection->send_close(1000);
  };

  client.on_open = [](shared_ptr<WsClient::Connection> connection) {
    cout << "Client: Opened connection" << endl;

    string message = "Hello";
    cout << "Client: Sending message: \"" << message << "\"" << endl;

    auto send_stream = make_shared<WsClient::SendStream>();
    *send_stream << message;
    connection->send(send_stream);
  };

  client.on_close = [](shared_ptr<WsClient::Connection> /*connection*/, int status, const string & /*reason*/) {
    cout << "Client: Closed connection with status code " << status << endl;
  };

  // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
  client.on_error = [](shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
    cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
  };

  client.start();
}
