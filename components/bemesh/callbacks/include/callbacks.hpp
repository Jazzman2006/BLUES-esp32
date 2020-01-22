#include <stdint.h>
#include "esp_log.h"
#include <unistd.h>


#define FUNCTOR_TAG "FUNCTOR"



#include "master.hpp"
#include "slave.hpp"
#include "routing.hpp"
#include "common.hpp"
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

extern "C"{
    #include "kernel.h"
}

namespace bemesh{
    extern bool discarded[SCAN_LIMIT];
    class Callback{
        

        static void init_callback(uint8_t type);
        static void notify_callback(uint16_t gattc_if,uint8_t conn_id,uint8_t charact,
                                    uint8_t* notify_data, uint8_t ntf_data_size);
        static void server_update_callback(uint8_t* macs,uint8_t flag,uint16_t gatt_if,
                                        uint8_t conn_id,uint8_t server_id);
        static void exchange_routing_table_callback(uint8_t* src,uint8_t* dest,
                                                uint16_t gatt_if,uint8_t conn_id);
        static void received_packet_callback(uint8_t* packet,uint16_t size);  
        static void shutdown_device_callback(uint8_t type);
        static void send_routing_table_callback(uint8_t* src,uint8_t* dst, uint16_t gatt_if,
                                                uint8_t conn_id,uint8_t server_id);

        static void endscanning_callback(device * device_list,uint8_t scan_seq,uint8_t flag_internal,
                                        uint8_t server_id);
        static void server_lost_callback(void);

        static void retry_callback(device * device_list,uint8_t scan_seq,uint8_t flag_internal,
                                        uint8_t server_id);

        //internal_client_id must be one of SERVER_S1, SERVER_S2 or SERVER_S3
        static void ssc_active_callback(uint8_t internal_client_id);

        static void ssc_passive_callback(uint8_t conn_id);
        


        static int choose_server(device* device_list, int device_list_size,
                             uint8_t internal_flag,uint8_t server_id,
                             connection_policy_t policy);
        
        static int connect_to_server(device* device_list, int device_list_size,
                                uint8_t internal_flag,uint8_t server_id,
                                connection_policy_t policy);

        static bool check_all_discarded();
        static void reset_discarded();

        public:
            void operator()(void);
            Callback();
    };
}



