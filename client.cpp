//
// Created by Itama on 23/07/2023.
//
#include "client.h"
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <cstring>
#include <algorithm>

#define CLIENT_TIMEOUT 5
#define BUFFER_SIZE 1024

void error_handling_2 (const std::string &msg, const std::string &scope)
{
  std::cerr << "system error: " << msg << " in function: " << scope << "\n";
  exit (1);
}

char *get_real_ip (const std::string &ip)
{
  // todo maybe the current ip is not in a good format, so here i'll change it
  return (char *) ip.c_str ();
}

void connect_by_socket (const std::string &ip, int port, live_server_info &
server)
{
  char *hostname = get_real_ip (ip);
  struct sockaddr_in sa;
  struct hostent *hp;
  int s;
  if ((hp = gethostbyname (hostname)) == NULL)
  { error_handling_2 (" couldn't get host address", "connect_by_socket"); }
  memset (&sa, 0, sizeof (sa));
//  memcpy ((char *) &sa.sin_addr, hp->h_addr, hp->h_length);
    inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
  sa.sin_family = AF_INET;
  sa.sin_port = htons ((u_short) port);
  //connect to a socket:
  if ((s = socket (AF_INET,
                   SOCK_STREAM, 0)) < 0)
  {
    error_handling_2 ("couldn't create socket for client",
                      "connect_by_socket");
  }
  struct timeval timeout;
  timeout.tv_sec = CLIENT_TIMEOUT;
  timeout.tv_usec = 0;
  fd_set writefds;
  FD_SET(s, &writefds);
  int res = select (s + 1, nullptr, &writefds, nullptr, &timeout);
  if (res < 0)
  {
    error_handling_2 ("couldn't use select function", "connect_by_socket");
  }
  if (res > 0){
      if (connect (s, (struct sockaddr *) &sa, sizeof (sa)) < 0)
      {
//          std::cout << strerror(errno) << "\n";
          close (s);
          error_handling_2 ("couldn't connect to server", "connect_by_socket"); // todo maybe i don't need to exit? maybe i can also do server.client_fd = -1?
      }
      //todo is s contain now the socket to the server?
      server.client_fd = s;
      return;
  }
  server.client_fd = -1;
}

void
connect_by_shm (const char *shm_pathname, int shm_proj_id, live_server_info &
server)
{
  key_t key;
  int shmid;
  if ((key = ftok (shm_pathname, shm_proj_id)) < 0)
  {
    //todo do i need to exit the program maybe?
//    error_handling_2 ("couldn't get key in ftok", "connect_by_shm");
    server.shmid = -1;
    return;
  }
  if ((shmid = shmget (key, SHARED_MEMORY_SIZE, 0666)) < 0)
  {
    //todo do i need to exit the program maybe?
    // error_handling_2 ("couldn't get shmid in shmget", "connect_by_shm");
    server.shmid = -1;
    return;
  }
  server.shmid = shmid;
}

void
read_info_file (const std::string &info_file_path, std::string &ip, int *port, std::string &
shm_pathname, int *shm_proj_id)
{
  std::ifstream info_file (info_file_path);
  if (!info_file)
  { error_handling_2 ("failed to open info file", "count_servers"); }
  std::string port_str, shm_proj_id_str;
  if (!(info_file >> ip >> port_str >> shm_pathname >> shm_proj_id_str))
  { error_handling_2 ("failed to read data from info file", "count_servers"); }
  *shm_proj_id = std::stoi (shm_proj_id_str);
  *port = std::stoi (port_str);
  info_file.close ();
}

void
connect_server (const std::string &info_file_path, live_server_info &server)
{
  std::string ip, shm_pathname_str;
  int shm_proj_id, port;
  read_info_file (info_file_path, ip, &port, shm_pathname_str,
                  &shm_proj_id);
  const char *shm_pathname = shm_pathname_str.c_str ();
  connect_by_socket (ip, port, server);
  connect_by_shm (shm_pathname, shm_proj_id, server);
}

int
count_servers (const std::string &client_files_directory, std::vector<live_server_info> &servers)
{
  int count_servs = 0;
  DIR *dir = opendir (client_files_directory.c_str ());
  if (!dir)
  { error_handling_2 ("not an actual directory", "count_servers"); }
  struct dirent *entry;
  while ((entry = readdir (dir)) != nullptr)
  {
    live_server_info server = {-1, 0, 0, ""};
    std::string fullPath = client_files_directory + "/" + entry->d_name;
    struct stat fileStat;
    if (stat (fullPath.c_str (), &fileStat) < 0)
    { continue; }
    if (S_ISDIR(fileStat.st_mode))// Skip directories
    { continue; }
    server.info_file_path = fullPath;
    connect_server (fullPath, server);
    count_servs++;
    servers.push_back (server);
  }
  std::sort (servers.begin (), servers.end (), [] (const live_server_info &a, const live_server_info &b)
  {
      return a.info_file_path < b.info_file_path;
  });
  closedir (dir);
  return count_servs;
}

void
print_by_format (unsigned int total_servers, int host, int container, int vm,
                 const std::vector<std::string> &messages)
{
  std::cout << "Total Servers: " << total_servers << "\n";
  std::cout << "Host: " << host << "\n";
  std::cout << "Container: " << container << "\n";
  std::cout << "VM: " << vm << "\n";
  std::cout << "Messages: ";
  for (const auto &msg: messages)
  {
    std::cout << msg << " ";
  }
  std::cout << "\n";
}


void print_server_infos (const std::vector<live_server_info> &servers)
{
  int host = 0, container = 0, vm = 0;
  unsigned int total_servers = servers.size ();
  std::vector<std::string> messages;
  for (const auto &server: servers)
  {
    bool shm_connected = false, socket_connected = false;
    std::string shm_msg;
    std::string sock_msg;
    get_message_from_shm(server, shm_msg);
    get_message_from_socket (server, sock_msg);
    if (!shm_msg.empty())
    {
      shm_connected = true;
      messages.push_back (shm_msg);
    }
    if (!sock_msg.empty())
    {
      socket_connected = true;
      messages.push_back (sock_msg);
    }
    if (shm_connected && socket_connected)
    { host++; }
    else if (socket_connected)
    { container++; }
    else
    { vm++; }
  }
  print_by_format (total_servers, host, container, vm, messages);
}
void get_message_from_socket (const live_server_info &server, std::string &msg)
{
  if (server.client_fd == -1)
  { return;}
  char buffer[SHARED_MEMORY_SIZE] = {};
  while (read (server.client_fd, buffer, SHARED_MEMORY_SIZE) > 0){
    msg += buffer;
    std::fill(buffer, buffer+ SHARED_MEMORY_SIZE, '\0');
  }
}
void get_message_from_shm (const live_server_info &server, std::string &msg)
{
  if (server.shmid == -1)
  { return;}
  char *msg_in = (char *) shmat (server.shmid, (void *) 0, 0);
  msg.assign (msg_in);
  shmdt (msg_in);
}
void disconnect (const std::vector<live_server_info> &servers)
{
  for (const auto& server: servers){
    if((close(server.client_fd)) != 0)
    { error_handling_2 ("failed to close socket", "disconnect");}
  }
}
void run (const std::string &client_files_directory)
{
  std::vector<live_server_info> servers;
  count_servers(client_files_directory, servers);
  print_server_infos (servers);
  disconnect (servers);
}





