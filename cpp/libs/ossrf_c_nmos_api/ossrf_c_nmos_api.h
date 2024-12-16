#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nmos_client nmos_client_t;

nmos_client_t* nmos_client_create(const char* node_configuration_location);

int nmos_client_add_device(nmos_client_t* client, const char* node_configuration_location);

int nmos_client_add_sender(nmos_client_t* client); 

int nmos_client_add_receiver(nmos_client_t* client); 

int nmos_client_remove_sender(nmos_client_t* client); 

int nmos_client_remove_receiver(nmos_client_t* client); 

#ifdef __cplusplus
}
#endif
