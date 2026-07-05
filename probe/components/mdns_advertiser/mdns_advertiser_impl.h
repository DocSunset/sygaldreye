/* Copyright 2025 Travis West */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle to an mDNS advertiser instance */
typedef struct mdns_adv_t mdns_adv_t;

mdns_adv_t* mdns_adv_create(const char* service_name, int port);
void        mdns_adv_destroy(mdns_adv_t* adv);
/* Process one round of incoming queries (call in a loop); timeout_ms <= 0 = non-blocking */
void        mdns_adv_poll(mdns_adv_t* adv, int timeout_ms);

#ifdef __cplusplus
}
#endif
