#include <iostream>
#include "client.h"
#include "server.h"
#include "globals.h"
int main ()
{
  std::vector<live_server_info> my_vec;
  count_servers ("../info_files_directory", my_vec);
}
