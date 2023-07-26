//
// Created by Itama on 26/07/2023.
//
#include "server.h"
#include "globals.h"
int main(){
    server_setup_information setupInformation;
    setupInformation.info_file_directory = "info_files_directory"; // run by clion: "../info_files_directory"
    setupInformation.port = 1234;
    setupInformation.info_file_name = "server1.txt"; // todo is it need to be txt file or something else?
    setupInformation.shm_pathname = "server1_shm.txt"; // run by clion: "../server1_shm.txt"
    setupInformation.shm_proj_id = 65;
    std::string shm_msg = "hiThisIsShmMessage";
    std::string sock_msg = "hiThisIsSOcketMessage";

    run(setupInformation, shm_msg, sock_msg);
}