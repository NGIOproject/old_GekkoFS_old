/*
  Copyright 2018-2019, Barcelona Supercomputing Center (BSC), Spain
  Copyright 2015-2019, Johannes Gutenberg Universitaet Mainz, Germany

  This software was partially supported by the
  EC H2020 funded project NEXTGenIO (Project ID: 671951, www.nextgenio.eu).

  This software was partially supported by the
  ADA-FS project under the SPPEXA project funded by the DFG.

  SPDX-License-Identifier: MIT
*/


#include <global/rpc/rpc_utils.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <system_error>

using namespace std;

/**
 * converts std bool to mercury bool
 * @param state
 * @return
 */
hg_bool_t bool_to_merc_bool(const bool state) {
    return state ? static_cast<hg_bool_t>(HG_TRUE) : static_cast<hg_bool_t>(HG_FALSE);
}


/**
 * Returns the machine's hostname
 * @return
 */
std::string get_my_hostname(bool short_hostname) {
    char hostname[1024];
    auto ret = gethostname(hostname, 1024);
    if (ret == 0) {
        std::string hostname_s(hostname);
        if (!short_hostname)
            return hostname_s;
        // get short hostname
        auto pos = hostname_s.find("."s);
        if (pos != std::string::npos)
            hostname_s = hostname_s.substr(0, pos);
        return hostname_s;
    } else
        return ""s;
}


string get_host_by_name(const string & hostname) {
    int err = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_RAW;

    struct addrinfo * addr = nullptr;

    err = getaddrinfo(
                hostname.c_str(),
                nullptr,
                &hints,
                &addr
          );
    if(err) {
        throw runtime_error("Error getting address info for '"
                            + hostname + "': " + gai_strerror(err));
    }

    char addr_str[INET6_ADDRSTRLEN];

    err = getnameinfo(
            addr->ai_addr, addr->ai_addrlen,
            addr_str,  INET6_ADDRSTRLEN,
            nullptr, 0,
            (NI_NUMERICHOST | NI_NOFQDN)
          );
    if (err) {
        throw runtime_error("Error on getnameinfo(): "s + gai_strerror(err));
    }
    freeaddrinfo(addr);
    return addr_str;
}

/**
 * checks if a Mercury handle's address is shared memory
 * @param mid
 * @param addr
 * @return bool
 */
bool is_handle_sm(margo_instance_id mid, const hg_addr_t& addr) {
    hg_size_t size = 128;
    char addr_cstr[128];
    if (margo_addr_to_string(mid, addr_cstr, &size, addr) != HG_SUCCESS)
        return false;
    string addr_str(addr_cstr);
    return addr_str.substr(0, 5) == "na+sm";
}