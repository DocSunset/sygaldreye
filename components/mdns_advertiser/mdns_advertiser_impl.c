/* Copyright 2025 Travis West */
#include "mdns_advertiser_impl.h"
#include "mdns.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 2048

struct mdns_adv_t {
    int    sock;
    char   service_type[64];   /* "_eyeballs._tcp.local." */
    char   instance_name[128]; /* "eyeballs._eyeballs._tcp.local." */
    char   hostname[64];       /* "eyeballs.local." */
    uint16_t port;
    struct sockaddr_in local_addr;
    char   buf[BUF_SIZE];
};

/* Get the first non-loopback IPv4 address */
static uint32_t get_local_ipv4(void) {
    struct ifaddrs* ifas = NULL;
    uint32_t result = htonl(INADDR_LOOPBACK);
    if (getifaddrs(&ifas) != 0) return result;
    for (struct ifaddrs* ifa = ifas; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
        struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
        if (sa->sin_addr.s_addr == htonl(INADDR_LOOPBACK)) continue;
        result = sa->sin_addr.s_addr;
        break;
    }
    freeifaddrs(ifas);
    return result;
}

static int query_callback(int sock, const struct sockaddr* from, size_t addrlen,
                          mdns_entry_type_t entry, uint16_t query_id,
                          uint16_t rtype, uint16_t rclass, uint32_t ttl,
                          const void* data, size_t size,
                          size_t name_offset, size_t name_length,
                          size_t record_offset, size_t record_length,
                          void* user_data) {
    (void)ttl; (void)size; (void)name_offset; (void)name_length;
    (void)record_offset; (void)record_length; (void)data;
    if (entry != MDNS_ENTRYTYPE_QUESTION) return 0;

    mdns_adv_t* adv = (mdns_adv_t*)user_data;

    mdns_record_t ptr = {0};
    ptr.name   = (mdns_string_t){adv->service_type, strlen(adv->service_type)};
    ptr.type   = MDNS_RECORDTYPE_PTR;
    ptr.data.ptr.name = (mdns_string_t){adv->instance_name, strlen(adv->instance_name)};
    ptr.ttl    = 120;
    ptr.rclass = MDNS_CLASS_IN;

    mdns_record_t srv = {0};
    srv.name   = (mdns_string_t){adv->instance_name, strlen(adv->instance_name)};
    srv.type   = MDNS_RECORDTYPE_SRV;
    srv.data.srv.port = adv->port;
    srv.data.srv.name = (mdns_string_t){adv->hostname, strlen(adv->hostname)};
    srv.ttl    = 120;
    srv.rclass = MDNS_CLASS_IN;

    mdns_record_t a = {0};
    a.name   = (mdns_string_t){adv->hostname, strlen(adv->hostname)};
    a.type   = MDNS_RECORDTYPE_A;
    a.data.a.addr.sin_family      = AF_INET;
    a.data.a.addr.sin_addr.s_addr = adv->local_addr.sin_addr.s_addr;
    a.ttl    = 120;
    a.rclass = MDNS_CLASS_IN;

    int unicast = rclass & MDNS_UNICAST_RESPONSE;
    if (rtype == MDNS_RECORDTYPE_PTR || rtype == MDNS_RECORDTYPE_ANY) {
        mdns_record_t additional[2] = {srv, a};
        if (unicast) {
            mdns_query_answer_unicast(sock, from, addrlen, adv->buf, BUF_SIZE,
                                     query_id, (mdns_record_type_t)rtype,
                                     adv->service_type, strlen(adv->service_type),
                                     ptr, NULL, 0, additional, 2);
        } else {
            mdns_query_answer_multicast(sock, adv->buf, BUF_SIZE, ptr, NULL, 0, additional, 2);
        }
    }
    return 0;
}

mdns_adv_t* mdns_adv_create(const char* service_name, int port) {
    mdns_adv_t* adv = (mdns_adv_t*)calloc(1, sizeof(*adv));
    if (!adv) return NULL;

    adv->port = (uint16_t)port;
    snprintf(adv->service_type,  sizeof(adv->service_type),  "_eyeballs._tcp.local.");
    snprintf(adv->instance_name, sizeof(adv->instance_name), "%s._eyeballs._tcp.local.", service_name);
    snprintf(adv->hostname,      sizeof(adv->hostname),      "%s.local.", service_name);

    adv->local_addr.sin_family      = AF_INET;
    adv->local_addr.sin_addr.s_addr = get_local_ipv4();
    adv->local_addr.sin_port        = htons(MDNS_PORT);

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family      = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port        = htons(MDNS_PORT);

    adv->sock = mdns_socket_open_ipv4(&saddr);
    if (adv->sock < 0) { free(adv); return NULL; }

    /* Initial announcement: PTR record, SRV+A as additional */
    mdns_record_t ptr = {0};
    ptr.name   = (mdns_string_t){adv->service_type, strlen(adv->service_type)};
    ptr.type   = MDNS_RECORDTYPE_PTR;
    ptr.data.ptr.name = (mdns_string_t){adv->instance_name, strlen(adv->instance_name)};
    ptr.ttl    = 120;
    ptr.rclass = MDNS_CLASS_IN;

    mdns_record_t srv = {0};
    srv.name   = (mdns_string_t){adv->instance_name, strlen(adv->instance_name)};
    srv.type   = MDNS_RECORDTYPE_SRV;
    srv.data.srv.port = adv->port;
    srv.data.srv.name = (mdns_string_t){adv->hostname, strlen(adv->hostname)};
    srv.ttl    = 120;
    srv.rclass = MDNS_CLASS_IN;

    mdns_record_t a = {0};
    a.name   = (mdns_string_t){adv->hostname, strlen(adv->hostname)};
    a.type   = MDNS_RECORDTYPE_A;
    a.data.a.addr.sin_family      = AF_INET;
    a.data.a.addr.sin_addr.s_addr = adv->local_addr.sin_addr.s_addr;
    a.ttl    = 120;
    a.rclass = MDNS_CLASS_IN;

    mdns_record_t additional[2] = {srv, a};
    mdns_announce_multicast(adv->sock, adv->buf, BUF_SIZE, ptr, NULL, 0, additional, 2);

    return adv;
}

void mdns_adv_destroy(mdns_adv_t* adv) {
    if (!adv) return;
    mdns_socket_close(adv->sock);
    free(adv);
}

void mdns_adv_poll(mdns_adv_t* adv, int timeout_ms) {
    if (!adv || adv->sock < 0) return;
    struct pollfd pfd = {adv->sock, POLLIN, 0};
    if (poll(&pfd, 1, timeout_ms) <= 0) return;
    mdns_socket_listen(adv->sock, adv->buf, BUF_SIZE, query_callback, adv);
}
