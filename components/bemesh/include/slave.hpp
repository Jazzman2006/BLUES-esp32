

#pragma once
#include "rtable.hpp" //For dev_addr_t data type.
#include <stdint.h>
#include <string>
#include  "gatts_table.h"
#include "message_handler.hpp"
#include "bemesh_messages.hpp"
#include "bemesh_status.hpp"
#include <stdlib.h>
#include <iostream>


extern "C"{
    #include "kernel.h"
}

namespace bemesh{
    static std::string comm_char = "Hello everyone";

    class Slave{
        uint8_t* address;
        uint16_t server_conn_id;
        uint16_t device_conn_id; // also for android compatibility mode.
        uint8_t device_gatt_if;
        bool esp;
        bool connected_to_internet;
        std::string name;
        MessageHandler msg_handler;



        public:
            Slave();
            ~Slave();
            Slave(bool is_esp, bool connected_to_internet);

            void start();
            void shutdown();

            std::string get_name();
            void set_name(std::string name);

            bool is_connected_to_internet();
            void set_connected_to_internet(bool connected_to_internet);

            bool is_esp();
            void set_esp(bool is_esp);

            uint8_t* get_dev_addr();
            void set_dev_addr(uint8_t* new_dev_addr);

            uint16_t get_server_connection_id();
            void set_server_connection_id(uint16_t conn_id);

            uint16_t get_device_connection_id();
            void set_device_connection_id(uint16_t device_conn_id);

            uint8_t get_device_gatt_if();
            void set_device_gatt_if(uint16_t gatt_if);


            MessageHandler get_message_handler();

            int16_t read_characteristic(uint8_t characteristic, dev_addr_t address,void* buffer,
                                        uint16_t buffer_size, uint16_t gattc_if,
                                        uint16_t conn_id);

            
            ErrStatus write_characteristic(uint8_t characteristic, dev_addr_t address, void* buffer,
                                        uint8_t buffer_size, uint16_t gattc_if,uint16_t conn_id);
            

            void sayHello(){
                std::cout<<"Hello"<<std::endl;
            }

            //This primitive directly interact with the communication characteristic (IDX_CHAR_A)
            ErrStatus send_message(uint16_t gattc_if, uint16_t conn_id, char* message,
                                    uint8_t message_size);


            //Util to print slave status and to verify correct data initialization.
            void print_status();


            

    };

    extern Slave* slave_istance;
}