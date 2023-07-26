//
// Created by Itama on 23/07/2023.
//
#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <fstream>
#include <sys/time.h>


#define MAX_CLIENTS 1
#define SERVER_TIMEOUT 120

void error_handling (const std::string &msg, const std::string &scope)
{
  std::cerr << "system error: " << msg << " in function: " << scope << "\n";
  exit (1);
}
void init_addr (sockaddr_in *server_addr, const server_setup_information
&setup_info, hostent *hp)
{
  char myname[MAXHOSTNAME + 1];
  gethostname (myname, MAXHOSTNAME);
  hp = gethostbyname (myname);
  if (hp == nullptr)
  { error_handling ("gethostbyname", "init_addr"); }
  char* ip = inet_ntoa(*((struct in_addr*) hp->h_addr))
  memset (server_addr, 0, sizeof (struct sockaddr_in));
  server_addr->sin_family = hp->h_addrtype;
  memcpy (&server_addr->sin_addr, hp->h_addr, hp->h_length);
  server_addr->sin_port = htons (setup_info.port);
}

void
start_communication (const server_setup_information &setup_info, live_server_info &server)
{
  // creating a socket:
  int shmid, socketfd;
  struct sockaddr_in server_addr;
  struct hostent *hp;
  key_t key;
  init_addr (&server_addr, setup_info, hp);
  if ((socketfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
  {
    error_handling ("socket", "start_communication");
  }
  if (bind (socketfd, (struct sockaddr *) &server_addr, sizeof (struct sockaddr_in))
      < 0)
  {
    close (socketfd);
    error_handling ("bind", "start_communication");
  }
  listen (socketfd, 3); // can be any other number greater than 1!
  const char* c_s_pathname = setup_info.shm_pathname.c_str();
  //creating shared memory segment:
  if ((key = ftok (c_s_pathname, setup_info.shm_proj_id)) < 0)
  {
    error_handling ("ftok", "start_communication");
  }
  if ((shmid = shmget (key, SHARED_MEMORY_SIZE, 0666 | IPC_CREAT)) < 0)
  {
    error_handling ("shmget", "start_communication");
  }
  server.shmid = shmid;
  server.server_fd = socketfd;

}
void
create_info_file (const server_setup_information &setup_info, live_server_info &server)
{
  std::ofstream info_file;
  struct sockaddr *addr;
  socklen_t addrlen;
  std::string path = setup_info.info_file_directory + setup_info
      .info_file_name;
  info_file.open (path, std::ios::out); // todo is path is okay? or should
  // todo be something else?
  if (!info_file)
  { error_handling ("open a file", "create_info_file"); }
  // todo is the following line okay??
  if (getpeername (server.server_fd, addr, &addrlen) < 0)
  { error_handling ("getpeername", "create_info_file"); }
  // todo if addr is not in the form of "x.y.z.w", need to deal with that.

  // writing to info file:
  info_file << addr->sa_data << "\n" << setup_info.port << "\n" <<
            setup_info.shm_pathname << "\n" << setup_info.shm_proj_id << "\n";
  server.info_file_path = path;
  info_file.close ();
}
void get_connection (live_server_info &server)
{
  int client_fd; /* socket of connection */
  struct timeval timeout;
  timeout.tv_sec = SERVER_TIMEOUT;
  timeout.tv_usec = 0;
  fd_set clientsfds;
  fd_set readfds;
  FD_ZERO(&clientsfds);
  FD_SET(server.server_fd, &clientsfds); // todo do i need this line?
  FD_SET(STDIN_FILENO, &clientsfds);
//  FD_SET(server.server_fd, &readfds); // todo do i need this line? i think not!

  if (select (MAX_CLIENTS + 1, nullptr, nullptr, nullptr, &timeout) < 0)
  {
    error_handling ("select", "get_connection");
  }
  if (FD_ISSET(server.server_fd, &readfds))
  {
    if ((client_fd = accept(server.server_fd, NULL, NULL)) < 0)
    { error_handling ("couldn't accept the call", "get_connection");}
    server.client_fd = client_fd;
    return;
  }
  server.client_fd = -1; // if no connection were established in 120 seconds
}
void write_to_socket (const live_server_info &server, const std::string &msg)
{
  size_t messageLength = msg.size();
  const char* messageToSend = msg.c_str();
  // todo should i write to server.server_fd or to server.client_fd
  if(write(server.client_fd, messageToSend, messageLength) == -1)
  { error_handling ("write", "write_to_socket");}
}
void write_to_shm (const live_server_info &server, const std::string &msg)
{
  size_t messageLength = msg.size();
  const char* messageToSend = msg.c_str();
  char* shm_addr = (char *)shmat (server.shmid, (void *)0, 0);
  memcpy (shm_addr, messageToSend, messageLength);
  shmdt (shm_addr);
}
void shutdown (const live_server_info &server)
{
  shmctl (server.shmid, IPC_RMID, NULL);
  close (server.server_fd);
  close (server.client_fd);
  const char * info_file_path = server.info_file_path.c_str();
  if (remove(info_file_path) != 0)
  { error_handling ("couldn't remove the info file", "shutdown");}
}
void
run (const server_setup_information &setup_info, const std::string &shm_msg, const std::string &socket_msg)
{
  live_server_info server = {0,0,0, ""};
  start_communication (setup_info, server);
  create_info_file (setup_info, server);
  write_to_shm (server, shm_msg);
  get_connection (server);
  if (server.client_fd != -1){
    write_to_socket (server, socket_msg);
  }
  sleep (10);
  shutdown (server);
}


