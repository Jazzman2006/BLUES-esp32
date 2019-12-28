#include "kernel.h"


//---------------------- CLIENT FUNCTIONS ----------------------//

// Task test in order to read and write
void my_task(void *pvParameters) {
    
    //esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *) pvParameters;
    int i = 0;
    for (; i<100; i++) {
		/*
		if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        */
        ESP_LOGI(GATTC_TAG, "READING");
        esp_err_t ret = esp_ble_gattc_read_char(gl_profile_tab2[PROFILE_A_APP_ID].gattc_if,
                                  gl_profile_tab2[PROFILE_A_APP_ID].conn_id,
                                  gl_profile_tab2[PROFILE_A_APP_ID].char_handle, ESP_GATT_AUTH_REQ_NONE);
                                  
        if (ret){
            ESP_LOGE(GATTC_TAG, "READ FAIL");
            break;
        }
        
        

		uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gl_profile_tab2[PROFILE_A_APP_ID].gattc_if,
                                  gl_profile_tab2[PROFILE_A_APP_ID].conn_id,
                                  gl_profile_tab2[PROFILE_A_APP_ID].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
		vTaskDelay(1000); // Waiting for 1000 ticks (not ms)
    }
    vTaskDelete(NULL);
}


void my_task2(void *pvParameters) {
	
	//vTaskDelay(1000); // Waiting for 1000 ticks (not ms)

    //uint8_t arr[13] = {3,3,3,3,3,3,3,3,3,3,3,3};
    uint8_t arr[8] = {1,1,1,1,1,1,1,1};
    //esp_ble_gattc_write_char(gl_profile_tab2[PROFILE_A_APP_ID].gattc_if, gl_profile_tab2[PROFILE_A_APP_ID].conn_id, CHR_HANDLES[IDX_CHAR_VAL_A], sizeof(arr), arr, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
	
	write_CHR(gl_internal_clients_tab[SERVER_S1].gattc_if, gl_internal_clients_tab[SERVER_S1].conn_id, IDX_CHAR_VAL_A, arr, 8);
	
	uint8_t * test = read_CHR(gl_internal_clients_tab[SERVER_S1].gattc_if, gl_internal_clients_tab[SERVER_S1].conn_id, IDX_CHAR_VAL_A);
	int i;

	//ESP_LOGE(GATTC_TAG, "ARRIVATO! LEN %d", get_CHR_value_len(IDX_CHAR_VAL_A));
	for(i=0; i<get_CHR_value_len(IDX_CHAR_VAL_A); i++) {
		ESP_LOGE(GATTC_TAG, "ELEM %d", test[i]);
	}  

    vTaskDelete(NULL);
}


static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GATTC_CONNECT_EVT:{
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gl_profile_tab2[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab2[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab2[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
      
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "service found");
            get_server = true;
            gl_profile_tab2[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab2[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab2[PROFILE_A_APP_ID].service_start_handle,
                                                                     gl_profile_tab2[PROFILE_A_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
			
            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab2[PROFILE_A_APP_ID].service_start_handle,
                                                             gl_profile_tab2[PROFILE_A_APP_ID].service_end_handle,
                                                             remote_filter_char_uuid,
                                                             char_elem_result,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab2[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab2[PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
         
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
      
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab2[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab2[PROFILE_A_APP_ID].service_start_handle,
                                                                         gl_profile_tab2[PROFILE_A_APP_ID].service_end_handle,
                                                                         gl_profile_tab2[PROFILE_A_APP_ID].char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gl_profile_tab2[PROFILE_A_APP_ID].conn_id,
                                                                         p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }
                    
                    /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                       
                        ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gl_profile_tab2[PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
						if (ret_status != ESP_GATT_OK){
							ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
						}
					
                    }
					
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }

        }
        
        
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
		esp_err_t ret;
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        if(p_data->notify.value[0] == 0xaa) {
			// Handles for characteristics
			if(p_data->notify.value_len == HRS_IDX_NB+1) {
				int i;
				for(i=1; i<p_data->notify.value_len; i++) {
					CHR_HANDLES[i-1] = p_data->notify.value[i];
				}
			}
			
			// Notifications on a characteristic => I'm going to read the char
			if(p_data->notify.value_len == 2) {
				ret = esp_ble_gattc_read_char(gattc_if, gl_profile_tab2[PROFILE_A_APP_ID].conn_id, p_data->notify.value[1], ESP_GATT_AUTH_REQ_NONE);
				if(ret) {
					ESP_LOGE(GATTC_TAG, "Error reading the char: %x", ret);
				}
			}
			
			
		}   

		
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
	}
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
        
        
        //xTaskCreate(my_task2, "TASK", 2048, NULL, 2, NULL);
        
        /*
        uint8_t write_char_data[35];
        
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_profile_tab2[PROFILE_A_APP_ID].conn_id,
                                  gl_profile_tab2[PROFILE_A_APP_ID].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
         */              
		//xTaskCreate(my_task, "TASK", 2048, param, 2, NULL); //https://www.freertos.org/a00125.html                        
        
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_CHAR_EVT");
        
        uint16_t handle = p_data->read.handle;
        uint8_t chr_idx = find_CHR(handle);
        
        CHR_ACT_LEN[chr_idx] = p_data->read.value_len;
        
        int i = 0;
        for(; i<p_data->read.value_len; i++) {
			//ESP_LOGI(GATTC_TAG, "read val %d: %x",i, p_data->read.value[i]);
			CHR_VALUES[chr_idx][i] = p_data->read.value[i];
		}	
        
        break;
    }
    
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        connect = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
        
        // Now i'm disconnected, trying to connect to become a server!
        //unregister_client();
        //gatt_server_main();
        esp_ble_gap_start_scanning(base_scan*scan_dividend);
        
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{	
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
		uint8_t mac[6];
		uint32_t SCAN_DURATION;
		esp_err_t ret = esp_efuse_mac_get_default(mac);
		
		if (ret) {
            ESP_LOGE(GATTC_TAG, "Cannot retrieve the MAC address, err %x", ret);
            break;
        }
	
		if(mac[0] == 0) {
			SCAN_DURATION = 1;
		} else {
			SCAN_DURATION = base_scan + mac[5]/scan_dividend;
		}
        
        ESP_LOGE(GATTC_TAG, "SCAN DURATION IS %d, MAC IS: %d:%d:%d:%d:%d:%d", SCAN_DURATION,
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        esp_ble_gap_start_scanning(SCAN_DURATION);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6); 
            ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);

#if CONFIG_EXAMPLE_DUMP_ADV_DATA_AND_SCAN_RESP
            if (scan_result->scan_rst.adv_data_len > 0) {
                ESP_LOGI(GATTC_TAG, "adv data:");
                esp_log_buffer_hex(GATTC_TAG, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
            }
            if (scan_result->scan_rst.scan_rsp_len > 0) {
                ESP_LOGI(GATTC_TAG, "scan resp:");
                esp_log_buffer_hex(GATTC_TAG, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
            }
#endif
			
            ESP_LOGI(GATTC_TAG, "\n");
			
            if (adv_name != NULL) {
				adv_name_len = DEVICE_NAME_LEN;
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
					int c = adv_name[CLIENTS_IDX] - '0';
					
					if(c > CLIENTS_NUMBER_LIMIT) {
						// The number of clients is over the limit
						ESP_LOGE(GATTC_TAG, "Too many clients here!");
						break;
					}
				
					
                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (connect == false) {
                        connect = true;
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab2[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
                    }
                }
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
			ESP_LOGE(GATTC_TAG, "I didn't find a server! I'm going to be a server...");
			
			unregister_client();
			gatt_server_main();
			
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop adv successfully");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab2[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab2[idx].gattc_if) {
                if (gl_profile_tab2[idx].gattc_cb) {
                    gl_profile_tab2[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}


//--------------------------------------------------------------//

//---------------------- SERVER FUNCTIONS ----------------------//

// Server task used to interconnect multiple servers
void server_task(void *pvParameters) {
	ESP_LOGE(GATTS_TAG, "ORA CHE SONO UN SERVER CERCO ALTRI SERVER!");
	
	esp_ble_gap_stop_advertising();
	server_is_busy = true;
	if(!conn_device_S1) {
		register_internal_client(SERVER_S1);
	} 
	
    vTaskDelete(NULL);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {	
    switch (event) {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        // REMOVE
        //esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        } else {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(GATTS_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(GATTS_TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(GATTS_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
		
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        /*
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;
		*/
		esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(device_name);
        if (set_dev_name_ret){
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret){
            ESP_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret){
            ESP_LOGE(GATTS_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

#endif
        //esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        
        esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        
        /* ---------------------------- */
        xTaskCreate(server_task, "ServerTask", 2048, NULL, 2, NULL); //https://www.freertos.org/a00125.html  
     case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_READ_EVT");
       	    break;
	 case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than GATTS_CHAR_VAL_LEN_MAX.
                ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
                if (heart_rate_handle_table[IDX_CHAR_CFG_A] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    // Used for the exchange of characteristics handles
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                        uint8_t notify_data[HRS_IDX_NB+1];
                        for (int i = 0; i < sizeof(notify_data); ++i) {
							if(!i) notify_data[i] = 0xaa;
							else notify_data[i] = heart_rate_handle_table[i-1]; //i % 0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
                                                sizeof(notify_data), notify_data, false);
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, heart_rate_handle_table[IDX_CHAR_VAL_A],
                                            sizeof(indicate_data), indicate_data, true);
                    }else if (descr_value == 0x0004){
						ESP_LOGE(GATTS_TAG, "A new server is connected");

						ID_TABLE[param->connect.conn_id] = SERVER;
						
						change_name(0, CLIENTS_IDX);
						change_name(1, SERVERS_IDX);
						esp_ble_gap_start_advertising(&adv_params);
                    
                    }else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                    }else{
                        ESP_LOGE(GATTS_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
                    }

                }
                
                // CHAR A: Someone is changing the value of the characteristic
                if (heart_rate_handle_table[IDX_CHAR_VAL_A] == param->write.handle) {
					// The value has been changed, I'm going to notify my neighbors
					int i;
					uint8_t indicate_data[2];
					indicate_data[0] = 0xaa;
					indicate_data[1] = heart_rate_handle_table[IDX_CHAR_VAL_A];

					for(i=0; i<TOTAL_NUMBER_LIMIT; ++i) {
						if(ID_TABLE[i] == SERVER || ID_TABLE[i] == CLIENT) {
							esp_ble_gatts_send_indicate(gatts_if, i, heart_rate_handle_table[IDX_CHAR_VAL_A],
												sizeof(indicate_data), indicate_data, true);
						}
					}
				}
				
				// CHAR B: Someone is changing the value of the characteristic
                if (heart_rate_handle_table[IDX_CHAR_VAL_B] == param->write.handle) {
					// The value has been changed, I'm going to notify my neighbors
					int i;
					uint8_t indicate_data[2];
					indicate_data[0] = 0xaa;
					indicate_data[1] = heart_rate_handle_table[IDX_CHAR_VAL_B];

					for(i=0; i<TOTAL_NUMBER_LIMIT; ++i) {
						if(ID_TABLE[i] == SERVER || ID_TABLE[i] == CLIENT) {
							esp_ble_gatts_send_indicate(gatts_if, i, heart_rate_handle_table[IDX_CHAR_VAL_B],
												sizeof(indicate_data), indicate_data, true);
						}
					}
				}
				
				// CHAR C: Someone is changing the value of the characteristic
                if (heart_rate_handle_table[IDX_CHAR_VAL_C] == param->write.handle) {
					// The value has been changed, I'm going to notify my neighbors
					int i;
					uint8_t indicate_data[2];
					indicate_data[0] = 0xaa;
					indicate_data[1] = heart_rate_handle_table[IDX_CHAR_VAL_C];

					for(i=0; i<TOTAL_NUMBER_LIMIT; ++i) {
						if(ID_TABLE[i] == SERVER || ID_TABLE[i] == CLIENT) {
							esp_ble_gatts_send_indicate(gatts_if, i, heart_rate_handle_table[IDX_CHAR_VAL_C],
												sizeof(indicate_data), indicate_data, true);
						}
					}
				}
				
                
                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* handle prepare write */
                example_prepare_write_event_env(gatts_if, &a_prepare_write_env, param);
            }
      	    break;
		case ESP_GATTS_EXEC_WRITE_EVT: 
            // the length of gattc prepare write data must be less than GATTS_CHAR_VAL_LEN_MAX. 
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&a_prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(GATTS_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            
            if(server_is_busy) break; // Server is actually looking for other servers to connect with
			
			n_connections++;
			change_name(1, CLIENTS_IDX);
			ID_TABLE[param->connect.conn_id] = CLIENT;
			
			//Restart the advs to be avaiable --> MULTIPLE CLIENTS
			esp_ble_gap_start_advertising(&adv_params);
        
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            ESP_LOGE(GATTS_TAG, "SI E' DISCONNESSO %d", param->disconnect.conn_id);
			ESP_LOGI(GATTS_TAG, "DISCONNECTED, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x",
						param->connect.conn_id,
							BDAS[param->disconnect.conn_id][0], BDAS[param->disconnect.conn_id][1], BDAS[param->disconnect.conn_id][2],
								BDAS[param->disconnect.conn_id][3], BDAS[param->disconnect.conn_id][4], BDAS[param->disconnect.conn_id][5]);
            
            int i;
			for(i=0; i<6; i++) {
				BDAS[param->disconnect.conn_id][i] = 0;
			}
			
			if(ID_TABLE[param->disconnect.conn_id] == SERVER) {
				change_name(0, SERVERS_IDX);
			} else if (ID_TABLE[param->disconnect.conn_id] == CLIENT) {
				change_name(0, CLIENTS_IDX);
			}
			
			n_connections--;
			
			esp_ble_gap_start_advertising(&adv_params);
				
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != HRS_IDX_NB){
                ESP_LOGE(GATTS_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
            }
            else {
                ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(heart_rate_handle_table, param->add_attr_tab.handles, sizeof(heart_rate_handle_table));
                esp_ble_gatts_start_service(heart_rate_handle_table[IDX_SVC]);
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[PROFILE_A_APP_ID].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

//--------------------------------------------------------------//

//---------------------- INTERNAL CLIENTS FUNCTIONS ----------------------//

static void start_scan(void) {
    stop_scan_done = false;
    Isconnecting = false;
    uint32_t duration = 15;
    esp_ble_gap_start_scanning(duration);
}

static void gattc_profile_S1_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{

    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    /* one device connect successfully, all profiles callback function will get the ESP_GATTC_CONNECT_EVT,
     so must compare the mac address to check which device is connected, so it is a good choice to use ESP_GATTC_OPEN_EVT. */
    case ESP_GATTC_CONNECT_EVT:
        break;
    case ESP_GATTC_OPEN_EVT:
        if (p_data->open.status != ESP_GATT_OK){
            //open failed, ignore the first device, connect the second device
            ESP_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
            conn_device_S1 = false;
			start_scan();
            break;
        }
        memcpy(gl_internal_clients_tab[SERVER_S1].remote_bda, p_data->open.remote_bda, 6);
        gl_internal_clients_tab[SERVER_S1].conn_id = p_data->open.conn_id;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        
        change_name(1,SERVERS_IDX);
        ID_TABLE[param->open.conn_id] = SERVER;
        server_is_busy = false;
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"Config mtu failed");
        }
        ESP_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service_S1 = true;
            gl_internal_clients_tab[SERVER_S1].service_start_handle = p_data->search_res.start_handle;
            gl_internal_clients_tab[SERVER_S1].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if (get_service_S1){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_internal_clients_tab[SERVER_S1].service_start_handle,
                                                                     gl_internal_clients_tab[SERVER_S1].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0) {
                char_elem_result_S1 = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result_S1){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else {
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_internal_clients_tab[SERVER_S1].service_start_handle,
                                                             gl_internal_clients_tab[SERVER_S1].service_end_handle,
                                                             remote_filter_char_uuid,
                                                             char_elem_result_S1,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result_S1[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_internal_clients_tab[SERVER_S1].char_handle = char_elem_result_S1[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_internal_clients_tab[SERVER_S1].remote_bda, char_elem_result_S1[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result_S1);
            }else {
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        uint16_t msg = 4;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     gl_internal_clients_tab[SERVER_S1].service_start_handle,
                                                                     gl_internal_clients_tab[SERVER_S1].service_end_handle,
                                                                     gl_internal_clients_tab[SERVER_S1].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0){
            descr_elem_result_S1 = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_elem_result_S1){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_elem_result_S1,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }
				
                /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                if (count > 0 && descr_elem_result_S1[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_S1[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                 descr_elem_result_S1[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }
                
                ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                 descr_elem_result_S1[0].handle,
                                                                 sizeof(msg),
                                                                 (uint8_t *)&msg,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }
                
				
                free(descr_elem_result_S1);
            }
        }
        else{
            ESP_LOGE(GATTC_TAG, "decsr not found");
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
		esp_err_t ret;
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        if(p_data->notify.value[0] == 0xaa) {
			// Handles for characteristics
			if(p_data->notify.value_len == HRS_IDX_NB+1) {
				int i;
				for(i=1; i<p_data->notify.value_len; i++) {
					CHR_HANDLES[i-1] = p_data->notify.value[i];
				}
			}
			
			// Notifications on a characteristic => I'm going to read the char
			if(p_data->notify.value_len == 2) {
				ret = esp_ble_gattc_read_char(gattc_if, gl_profile_tab2[PROFILE_A_APP_ID].conn_id, p_data->notify.value[1], ESP_GATT_AUTH_REQ_NONE);
				if(ret) {
					ESP_LOGE(GATTC_TAG, "Error reading the char: %x", ret);
				}
			}
			
			
		}   

		
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
	}
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success");
        
        //xTaskCreate(my_task2, "TASK", 2048, NULL, 2, NULL);
        /*
        uint8_t write_char_data[35];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char( gattc_if,
                                  gl_internal_clients_tab[SERVER_S1].conn_id,
                                  gl_internal_clients_tab[SERVER_S1].char_handle,
                                  sizeof(write_char_data),
                                  write_char_data,
                                  ESP_GATT_WRITE_TYPE_RSP,
                                  ESP_GATT_AUTH_REQ_NONE);
        */
        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
        }else{
            ESP_LOGI(GATTC_TAG, "write char success");
        }
        //start_scan();
        break;
    case ESP_GATTC_READ_CHAR_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_CHAR_EVT");
        
        uint16_t handle = p_data->read.handle;
        uint8_t chr_idx = find_CHR(handle);
        
        CHR_ACT_LEN[chr_idx] = p_data->read.value_len;
        
        int i = 0;
        for(; i<p_data->read.value_len; i++) {
			//ESP_LOGI(GATTC_TAG, "read val %d: %x",i, p_data->read.value[i]);
			CHR_VALUES[chr_idx][i] = p_data->read.value[i];
		}	
        
        break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
        //Start scanning again
        //start_scan();
        
        if (memcmp(p_data->disconnect.remote_bda, gl_internal_clients_tab[SERVER_S1].remote_bda, 6) == 0){
            ESP_LOGI(GATTC_TAG, "device a disconnect");
            conn_device_S1 = false;
            get_service_S1 = false;
            esp_ble_gap_start_scanning(base_scan*scan_dividend);
            //change_name(0, SERVERS_IDX);
        }
        break;
    default:
        break;
    }
}

static void gattc_profile_S2_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        break;
    case ESP_GATTC_CONNECT_EVT:
        break;
    case ESP_GATTC_OPEN_EVT:
        if (p_data->open.status != ESP_GATT_OK){
            //open failed, ignore the second device, connect the third device
            ESP_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
            conn_device_S2 = false;
			start_scan();
            break;
        }
        memcpy(gl_internal_clients_tab[SERVER_S2].remote_bda, p_data->open.remote_bda, 6);
        gl_internal_clients_tab[SERVER_S2].conn_id = p_data->open.conn_id;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        
        change_name(1,SERVERS_IDX);
        ID_TABLE[param->open.conn_id] = SERVER;
        server_is_busy = false;
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"Config mtu failed");
        }
        ESP_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service_S2 = true;
            gl_internal_clients_tab[SERVER_S2].service_start_handle = p_data->search_res.start_handle;
            gl_internal_clients_tab[SERVER_S2].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if (get_service_S2){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_internal_clients_tab[SERVER_S2].service_start_handle,
                                                                     gl_internal_clients_tab[SERVER_S2].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                char_elem_result_S2 = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result_S2){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_internal_clients_tab[SERVER_S2].service_start_handle,
                                                             gl_internal_clients_tab[SERVER_S2].service_end_handle,
                                                             remote_filter_char_uuid,
                                                             char_elem_result_S2,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result_S2[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_internal_clients_tab[SERVER_S2].char_handle = char_elem_result_S2[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_internal_clients_tab[SERVER_S2].remote_bda, char_elem_result_S2[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result_S2);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {

        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        uint16_t msg = 4;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     gl_internal_clients_tab[SERVER_S2].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     gl_internal_clients_tab[SERVER_S2].service_start_handle,
                                                                     gl_internal_clients_tab[SERVER_S2].service_end_handle,
                                                                     gl_internal_clients_tab[SERVER_S2].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0){
            descr_elem_result_S2 = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_elem_result_S2){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_elem_result_S1,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }
				
                /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                if (count > 0 && descr_elem_result_S1[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_S1[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                 descr_elem_result_S1[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }
                
                ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                 descr_elem_result_S1[0].handle,
                                                                 sizeof(msg),
                                                                 (uint8_t *)&msg,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }
                
				
                /* free descr_elem_result */
                free(descr_elem_result_S2);
            }
        }
        else{
            ESP_LOGE(GATTC_TAG, "decsr not found");
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
		esp_err_t ret;
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        if(p_data->notify.value[0] == 0xaa) {
			// Handles for characteristics
			if(p_data->notify.value_len == HRS_IDX_NB+1) {
				int i;
				for(i=1; i<p_data->notify.value_len; i++) {
					CHR_HANDLES[i-1] = p_data->notify.value[i];
				}
			}
			
			// Notifications on a characteristic => I'm going to read the char
			if(p_data->notify.value_len == 2) {
				ret = esp_ble_gattc_read_char(gattc_if, gl_profile_tab2[PROFILE_A_APP_ID].conn_id, p_data->notify.value[1], ESP_GATT_AUTH_REQ_NONE);
				if(ret) {
					ESP_LOGE(GATTC_TAG, "Error reading the char: %x", ret);
				}
			}
			
			
		}   

        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
	}
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success");
        
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
        }else{
            ESP_LOGI(GATTC_TAG, "Write char success");
        }
        //start_scan();
        break;
        
    case ESP_GATTC_READ_CHAR_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_CHAR_EVT");
        
        uint16_t handle = p_data->read.handle;
        uint8_t chr_idx = find_CHR(handle);
        
        CHR_ACT_LEN[chr_idx] = p_data->read.value_len;
        
        int i = 0;
        for(; i<p_data->read.value_len; i++) {
			//ESP_LOGI(GATTC_TAG, "read val %d: %x",i, p_data->read.value[i]);
			CHR_VALUES[chr_idx][i] = p_data->read.value[i];
		}	
        
        break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
		
        if (memcmp(p_data->disconnect.remote_bda, gl_internal_clients_tab[SERVER_S2].remote_bda, 6) == 0){
            ESP_LOGI(GATTC_TAG, "device b disconnect");
            conn_device_S2 = false;
            get_service_S2 = false;
            esp_ble_gap_start_scanning(base_scan*scan_dividend);
            //change_name(0, SERVERS_IDX);
        }
        break;
    default:
        break;
    }
}

static void gattc_profile_S3_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        break;
    case ESP_GATTC_CONNECT_EVT:
        break;
    case ESP_GATTC_OPEN_EVT:
        if (p_data->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
            conn_device_S3 = false;
            start_scan();
            break;
        }
        memcpy(gl_internal_clients_tab[SERVER_S3].remote_bda, p_data->open.remote_bda, 6);
        gl_internal_clients_tab[SERVER_S3].conn_id = p_data->open.conn_id;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        
        change_name(1,SERVERS_IDX);
        ID_TABLE[param->open.conn_id] = SERVER;
        server_is_busy = false;
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"Config mtu failed");
        }
        ESP_LOGI(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID) {
            ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
            get_service_S3 = true;
            gl_internal_clients_tab[SERVER_S3].service_start_handle = p_data->search_res.start_handle;
            gl_internal_clients_tab[SERVER_S3].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if (get_service_S3){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_internal_clients_tab[SERVER_S3].service_start_handle,
                                                                     gl_internal_clients_tab[SERVER_S3].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                char_elem_result_S3 = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result_S3){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_internal_clients_tab[SERVER_S3].service_start_handle,
                                                             gl_internal_clients_tab[SERVER_S3].service_end_handle,
                                                             remote_filter_char_uuid,
                                                             char_elem_result_S3,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result_S3[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_internal_clients_tab[SERVER_S3].char_handle = char_elem_result_S3[0].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_internal_clients_tab[SERVER_S3].remote_bda, char_elem_result_S3[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result_S3);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        uint16_t msg = 4;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     gl_internal_clients_tab[SERVER_S3].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     gl_internal_clients_tab[SERVER_S3].service_start_handle,
                                                                     gl_internal_clients_tab[SERVER_S3].service_end_handle,
                                                                     gl_internal_clients_tab[SERVER_S3].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0){
            descr_elem_result_S3 = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_elem_result_S3){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_elem_result_S1,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }
				
                /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                if (count > 0 && descr_elem_result_S1[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_S1[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                 descr_elem_result_S1[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }
                
                ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_internal_clients_tab[SERVER_S1].conn_id,
                                                                 descr_elem_result_S1[0].handle,
                                                                 sizeof(msg),
                                                                 (uint8_t *)&msg,
                                                                 ESP_GATT_WRITE_TYPE_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }
                
				

                /* free descr_elem_result */
                free(descr_elem_result_S3);
            }
        }
        else{
            ESP_LOGE(GATTC_TAG, "decsr not found");
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
		esp_err_t ret;
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        if(p_data->notify.value[0] == 0xaa) {
			// Handles for characteristics
			if(p_data->notify.value_len == HRS_IDX_NB+1) {
				int i;
				for(i=1; i<p_data->notify.value_len; i++) {
					CHR_HANDLES[i-1] = p_data->notify.value[i];
				}
			}
			
			// Notifications on a characteristic => I'm going to read the char
			if(p_data->notify.value_len == 2) {
				ret = esp_ble_gattc_read_char(gattc_if, gl_profile_tab2[PROFILE_A_APP_ID].conn_id, p_data->notify.value[1], ESP_GATT_AUTH_REQ_NONE);
				if(ret) {
					ESP_LOGE(GATTC_TAG, "Error reading the char: %x", ret);
				}
			}
			
			
		}   

        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
	}
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success");
        
        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "Write char success");
        //start_scan();
        break;
    case ESP_GATTC_READ_CHAR_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_CHAR_EVT");
        
        uint16_t handle = p_data->read.handle;
        uint8_t chr_idx = find_CHR(handle);
        
        CHR_ACT_LEN[chr_idx] = p_data->read.value_len;
        
        int i = 0;
        for(; i<p_data->read.value_len; i++) {
			//ESP_LOGI(GATTC_TAG, "read val %d: %x",i, p_data->read.value[i]);
			CHR_VALUES[chr_idx][i] = p_data->read.value[i];
		}	
        
        break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
		
        if (memcmp(p_data->disconnect.remote_bda, gl_internal_clients_tab[SERVER_S3].remote_bda, 6) == 0){
            ESP_LOGI(GATTC_TAG, "device c disconnect");
            conn_device_S3 = false;
            get_service_S3 = false;
            esp_ble_gap_start_scanning(base_scan*scan_dividend);
            //change_name(0, SERVERS_IDX);
        }
        break;
    default:
        break;
    }
}


static void esp_gap_S1_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 15;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		int my_s = device_name[SERVERS_IDX] - '0';
		// Max number of servers has been reached
		if(my_s == SERVERS_NUMBER_LIMIT) {
			stop_scan_done = true;
            esp_ble_gap_stop_scanning();
            break;
		}
		
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
            if (Isconnecting){
                break;
            }
            if (conn_device_S1 && conn_device_S2 && conn_device_S3 && !stop_scan_done){
                stop_scan_done = true;
                esp_ble_gap_stop_scanning();
                ESP_LOGI(GATTC_TAG, "all devices are connected");
                break;
            }
            if (adv_name != NULL) {
				adv_name_len = DEVICE_NAME_LEN;
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
					int s = adv_name[SERVERS_IDX] - '0';
					if(s == SERVERS_NUMBER_LIMIT) {
						// The number of clients is over the limit
						ESP_LOGE(GATTC_TAG, "Too many servers here!");
						break;
					}	
					int i,k;
					for(i=0; i<TOTAL_NUMBER_LIMIT; i++) {
						uint8_t check = 1;
						for(k=0; k<6; k++) {
							if(BDAS[i][k] != scan_result->scan_rst.bda[k]) check = 0;
						}
						if(check) break; // This BDA is already in the list of connected devices
					}			
					
					
					ESP_LOGI(GATTC_TAG, "searched device %s, connect is %d\n", remote_device_name,conn_device_S1);
                    if (conn_device_S1 == false) {
                        conn_device_S1 = true;
                        stop_scan_done = true;
						esp_ble_gap_stop_scanning();
						
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gattc_open(gl_internal_clients_tab[SERVER_S1].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
						Isconnecting = true;
						esp_ble_gap_start_advertising(&adv_params);
                    }
				}
			}

            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
			// Once the search is end we unregister the client
			stop_scan_done = true;
            esp_ble_gap_stop_scanning();
			//unregister_internal_client(SERVER_S1);
			esp_ble_gap_start_advertising(&adv_params);
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

static void esp_gap_S2_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 15;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		int my_s = device_name[SERVERS_IDX] - '0';
		// Max number of servers has been reached
		if(my_s == 3) {
			stop_scan_done = true;
            esp_ble_gap_stop_scanning();
            break;
		}
		
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
       case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
            if (Isconnecting){
                break;
            }
            if (conn_device_S1 && conn_device_S2 && conn_device_S3 && !stop_scan_done){
                stop_scan_done = true;
                esp_ble_gap_stop_scanning();
                ESP_LOGI(GATTC_TAG, "all devices are connected");
                break;
            }
            if (adv_name != NULL) {
				adv_name_len = DEVICE_NAME_LEN;
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
					int s = adv_name[SERVERS_IDX] - '0';
					if(s == SERVERS_NUMBER_LIMIT) {
						// The number of clients is over the limit
						ESP_LOGE(GATTC_TAG, "Too many servers here!");
						break;
					}	
					int i,k;
					for(i=0; i<TOTAL_NUMBER_LIMIT; i++) {
						uint8_t check = 1;
						for(k=0; k<6; k++) {
							if(BDAS[i][k] != scan_result->scan_rst.bda[k]) check = 0;
						}
						if(check) break; // This BDA is already in the list of connected devices
					}			
					
					
					ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (conn_device_S2 == false) {
                        conn_device_S2 = true;
                        stop_scan_done = true;
						esp_ble_gap_stop_scanning();
						
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gattc_open(gl_internal_clients_tab[SERVER_S1].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
						Isconnecting = true;
						esp_ble_gap_start_advertising(&adv_params);
                    }
				}
			}

            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
			// Once the search is end we unregister the client
			stop_scan_done = true;
            esp_ble_gap_stop_scanning();
			unregister_internal_client(SERVER_S2);
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

static void esp_gap_S3_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 15;
        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
		int my_s = device_name[SERVERS_IDX] - '0';
		// Max number of servers has been reached
		if(my_s == 3) {
			stop_scan_done = true;
            esp_ble_gap_stop_scanning();
            break;
		}
		
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
            if (Isconnecting){
                break;
            }
            if (conn_device_S1 && conn_device_S2 && conn_device_S3 && !stop_scan_done){
                stop_scan_done = true;
                esp_ble_gap_stop_scanning();
                ESP_LOGI(GATTC_TAG, "all devices are connected");
                break;
            }
            if (adv_name != NULL) {
				adv_name_len = DEVICE_NAME_LEN;
                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
					int s = adv_name[SERVERS_IDX] - '0';
					if(s == SERVERS_NUMBER_LIMIT) {
						// The number of clients is over the limit
						ESP_LOGE(GATTC_TAG, "Too many servers here!");
						break;
					}	
					int i,k;
					for(i=0; i<TOTAL_NUMBER_LIMIT; i++) {
						uint8_t check = 1;
						for(k=0; k<6; k++) {
							if(BDAS[i][k] != scan_result->scan_rst.bda[k]) check = 0;
						}
						if(check) break; // This BDA is already in the list of connected devices
					}			
					
					
					ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                    if (conn_device_S3 == false) {
                        conn_device_S3 = true;
                        stop_scan_done = true;
						esp_ble_gap_stop_scanning();
						
                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
                        esp_ble_gattc_open(gl_internal_clients_tab[SERVER_S1].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
						Isconnecting = true;
						esp_ble_gap_start_advertising(&adv_params);
                    }
				}
			}

            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
			// Once the search is end we unregister the client
			stop_scan_done = true;
            esp_ble_gap_stop_scanning();
			unregister_internal_client(SERVER_S3);
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_internal_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    //ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);

     //If event is register event, store the gattc_if for each profile 
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_internal_clients_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

     //If the gattc_if equal to profile A, call profile A cb handler,
      //so here call each profile's callback
    do {
        int idx;
        for (idx = 0; idx < SERVERS_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || //ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function
                    gattc_if == gl_internal_clients_tab[idx].gattc_if) {
                if (gl_internal_clients_tab[idx].gattc_cb) {
                    gl_internal_clients_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

//------------------------------------------------------------------------//


static uint8_t get_num_connections() {
	return n_connections;
}

static uint8_t** get_connected_BDAS() {
	return BDAS;
}

static uint8_t get_type_connection(uint8_t conn_id) {
	switch(ID_TABLE[conn_id]) {
	case SERVER:
		return SERVER;
	case CLIENT:
		return CLIENT;
	default:
		return NOID;
	}
}

static void start_internal_client(uint8_t client) {
	esp_ble_gap_stop_advertising();
	server_is_busy = true;
	switch(client) {
	case SERVER_S1:
		if(!conn_device_S1) {
			register_internal_client(SERVER_S1);
		}
		break;
	case SERVER_S2:
		if(!conn_device_S2) {
			register_internal_client(SERVER_S2);
		}
		break;
	case SERVER_S3:
		if(!conn_device_S3) {
			register_internal_client(SERVER_S3);
		}
		break;
	default:
		break;
	}
}

static void write_CHR(uint16_t gattc_if, uint16_t conn_id, uint8_t chr, uint8_t* array, uint8_t len) {
		
		if(!array) return;
		esp_err_t ret = esp_ble_gattc_write_char(gattc_if, conn_id, CHR_HANDLES[chr], len, array, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
		
		if(ret) {
			ESP_LOGE(GATTC_TAG, "Error writing the char: %x", ret);
		}
		vTaskDelay(200);
}

static uint8_t* read_CHR(uint16_t gattc_if, uint16_t conn_id, uint8_t chr) {
	
	esp_err_t ret = esp_ble_gattc_read_char(gattc_if, conn_id, CHR_HANDLES[chr], ESP_GATT_AUTH_REQ_NONE);
	if(ret) {
		ESP_LOGE(GATTC_TAG, "Error reading the char: %x", ret);
	}
	
	vTaskDelay(200); // Waiting for 500 ticks (not ms)
	
	return CHR_VALUES[chr];
}

static uint8_t get_CHR_value_len(uint8_t chr) {
	return CHR_ACT_LEN[chr];
}

static uint8_t find_CHR(uint16_t handle) {
	int i;
	for(i=0; i<HRS_IDX_NB; ++i) {
		if(CHR_HANDLES[i] == handle) return i;
	}
	return NOID;
}

static void change_name(uint8_t flag, uint8_t idx) {
	int i = device_name[idx] - '0';
	if(flag) {
        i++;
	} else {
		i--;
	}
	char c = i + '0';
	device_name[idx] = c;
	
	esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(device_name);
	if (set_dev_name_ret){
		ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
	}
	
	//config adv data
	esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
	if (ret){
		ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
	}
	adv_config_done |= adv_config_flag;
	//config scan response data
	ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
	if (ret){
		ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
	}
	adv_config_done |= scan_rsp_config_flag;
}

static void register_internal_client(uint8_t client_num) {
	esp_err_t ret;
	
    //register the  callback function to the gap module
    switch(client_num) {
	case SERVER_S1:
		ret = esp_ble_gap_register_callback(esp_gap_S1_cb);
		break;
	case SERVER_S2:
		ret = esp_ble_gap_register_callback(esp_gap_S2_cb);
		break;
	case SERVER_S3:
		ret = esp_ble_gap_register_callback(esp_gap_S3_cb);
		break;
	default:
		ret = 1;
        break;
	}
	
    if (ret){
        ESP_LOGE(GATTC_TAG, "gap register error, error code = %x", ret);
        return;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_internal_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "gattc register error, error code = %x", ret);
        return;
    }
	
	ret = esp_ble_gattc_app_register(client_num);
    if (ret){
        ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
        return;
    }
}

static void unregister_internal_client(uint8_t client_num) {
	ESP_LOGE(GATTC_TAG, "CLOSING CLIENT %d", client_num);
	esp_err_t ret;
	
	ret = esp_ble_gattc_app_unregister(gl_internal_clients_tab[client_num].gattc_if);
	if (ret){
		ESP_LOGE(GATTC_TAG, "%s gap unregister failed, error code = %x\n", __func__, ret);
		return;
	}
	
}

static void gatt_client_main() {
	ESP_LOGE(GATTC_TAG, "I'M A CLIENT!!!");
	
	esp_err_t ret;
	//register the  callback function to the gap module
	ret = esp_ble_gap_register_callback(esp_gap_cb);
	if (ret){
		ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
		return;
	}
	

	//register the callback function to the gattc module
	ret = esp_ble_gattc_register_callback(esp_gattc_cb);
	if(ret){
		ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
		return;
	}

	ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
	if (ret){
		ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
	}
	

	esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
	if (local_mtu_ret){
		ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
	}
}

static void gatt_server_main() {
	ESP_LOGE(GATTC_TAG, "I'M A SERVER!!!");
    esp_err_t ret;
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    
    
	ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
	if (ret){
		ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
		return;
	}
	/*
	ret = esp_ble_gatts_app_register(PROFILE_B_APP_ID);
	if (ret){
		ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
		return;
	}
	*/
	
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
	
}

static void unregister_client() {
	ESP_LOGE(GATTC_TAG, "REMOVING CLIENT!");
	esp_err_t ret;
	
	ret = esp_ble_gattc_app_unregister(gl_profile_tab2[PROFILE_A_APP_ID].gattc_if);
	if (ret){
		ESP_LOGE(GATTC_TAG, "%s gap unregister failed, error code = %x\n", __func__, ret);
		return;
	}
	
}

static void unregister_server() {
	ESP_LOGE(GATTC_TAG, "REMOVING SERVER!");
	
	esp_err_t ret;
	
	ret = esp_ble_gatts_app_unregister(gl_profile_tab2[PROFILE_A_APP_ID].gattc_if);
	if (ret){
		ESP_LOGE(GATTC_TAG, "%s gap unregister failed, error code = %x\n", __func__, ret);
		return;
	}
	/*
	ret = esp_ble_gatts_app_unregister(gl_profile_tab2[PROFILE_B_APP_ID].gattc_if);
	if (ret){
		ESP_LOGE(GATTC_TAG, "%s gap unregister failed, error code = %x\n", __func__, ret);
		return;
	}
	*/
}

static void ble_esp_startup() {
	esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    
}

/* 
// Main, used for debug
void app_main() {
	ble_esp_startup();
    
	//gatt_client_main();
	gatt_server_main();
}
*/