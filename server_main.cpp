//
// Created by Itama on 26/07/2023.
//
#include "server.h"
#include "globals.h"
int main(){
    server_setup_information setupInformation;
    setupInformation.info_file_directory = "info_files_directory"; // terminal
//    setupInformation.info_file_directory = "../info_files_directory"; //clion
    setupInformation.port = 3001;
    setupInformation.info_file_name = "server1.txt";
    setupInformation.shm_pathname = "server1_shm.txt"; // terminal
//    setupInformation.shm_pathname = "../server1_shm.txt"; // clion
    setupInformation.shm_proj_id = 65;
    std::string shm_msg = "hiThisIsShmMessage";
    std::string sock_msg = "hiThisIsSOcketMessage";

    run(setupInformation, shm_msg, sock_msg);
}